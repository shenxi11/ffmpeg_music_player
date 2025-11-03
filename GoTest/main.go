package main

/*
#cgo CFLAGS: -I.
#cgo LDFLAGS: -L. -l:library.so -lmysqlcppconn -lmysqlclient
#include <stdlib.h>
#include "library.h"
*/
import "C"
import (
	"context"
	"crypto/md5"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"log"
	"net/http"
	"net/url"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
	"time"
	"unsafe"

	"github.com/go-redis/redis/v8"
	"gopkg.in/yaml.v2"
)

var UPLOAD_FOLDER string
var STATIC_FOLDER string
var MAX_FILE_SIZE = 1024 * 1024 * 1024 // 1GB

type Config struct {
	Server struct {
		Host      string `yaml:"host"`
		Port      int    `yaml:"port"`
		StaticDir string `yaml:"static_dir"`
		UploadDir string `yaml:"upload_dir"`
	} `yaml:"server"`
	Database struct {
		Host     string `yaml:"host"`
		Port     int    `yaml:"port"`
		User     string `yaml:"user"`
		Password string `yaml:"password"`
		Name     string `yaml:"name"`
	} `yaml:"database"`
	Redis struct {
		Host     string `yaml:"host"`
		Port     int    `yaml:"port"`
		Password string `yaml:"password"`
	} `yaml:"redis"`
}

var config Config

// Redis client
var rdb *redis.Client
var ctx = context.Background()

// JSON structures
type StreamRequest struct {
	Filename string `json:"filename"`
}

type StreamResponse struct {
	Message       string   `json:"message"`
	StreamURL     string   `json:"stream_url"`
	LrcURL        *string  `json:"lrc_url"`
	AlbumCoverURL *string  `json:"album_cover_url"`
	ID            string   `json:"ID"`
	Duration      *float64 `json:"duration"`
}

type FileSearchRequest struct {
	Filename string `json:"filename"`
}

type UserRequest struct {
	Password string `json:"password"`
	Account  string `json:"account"`
	Username string `json:"username"`
}

type LoginResponse struct {
	Success      string   `json:"success"`
	SongPathList []string `json:"song_path_list"`
}

type AddMusicRequest struct {
	Username  string `json:"username"`
	MusicPath string `json:"music_path"`
}

// Cache keys
const (
	CACHE_PREFIX_FILES    = "files:"
	CACHE_PREFIX_USER     = "user:"
	CACHE_PREFIX_SONG     = "song:"
	CACHE_PREFIX_DURATION = "duration:"
	CACHE_TTL_SHORT       = 5 * time.Minute  // 短期缓存
	CACHE_TTL_MEDIUM      = 30 * time.Minute // 中期缓存
	CACHE_TTL_LONG        = 2 * time.Hour    // 长期缓存
)

// Utility functions
func initRedis() {
	addr := fmt.Sprintf("%s:%d", config.Redis.Host, config.Redis.Port)
	rdb = redis.NewClient(&redis.Options{
		Addr:     addr,
		Password: config.Redis.Password,
		DB:       0, // default DB
	})
	// Test connection
	_, err := rdb.Ping(ctx).Result()
	if err != nil {
		log.Printf("Redis connection failed: %v", err)
	} else {
		log.Println("Redis connected successfully")
	}
}

func generateSongID(filePath string) (string, error) {
	// Try cache first
	cacheKey := CACHE_PREFIX_SONG + filePath
	if cachedID, err := rdb.Get(ctx, cacheKey).Result(); err == nil {
		return cachedID, nil
	}

	file, err := os.Open(filePath)
	if err != nil {
		return "", err
	}
	defer file.Close()

	hash := md5.New()
	if _, err := io.Copy(hash, file); err != nil {
		return "", err
	}

	songID := fmt.Sprintf("%x", hash.Sum(nil))

	// Cache result
	rdb.Set(ctx, cacheKey, songID, CACHE_TTL_LONG)

	return songID, nil
}

func getAudioDuration(filePath string) (*float64, error) {
	// Try cache first
	cacheKey := CACHE_PREFIX_DURATION + filePath
	if cachedDuration, err := rdb.Get(ctx, cacheKey).Result(); err == nil {
		if duration, err := strconv.ParseFloat(cachedDuration, 64); err == nil {
			return &duration, nil
		}
	}

	cmd := exec.Command("ffprobe", "-v", "error", "-show_entries", "format=duration",
		"-of", "default=noprint_wrappers=1:nokey=1", filePath)

	output, err := cmd.Output()
	if err != nil {
		return nil, err
	}

	durationStr := strings.TrimSpace(string(output))
	duration, err := strconv.ParseFloat(durationStr, 64)
	if err != nil {
		return nil, err
	}

	// Cache result
	rdb.Set(ctx, cacheKey, durationStr, CACHE_TTL_LONG)

	return &duration, nil
}

func enableCORS(w http.ResponseWriter) {
	w.Header().Set("Access-Control-Allow-Origin", "*")
	w.Header().Set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
	w.Header().Set("Access-Control-Allow-Headers", "Content-Type")
}

func fileExists(filename string) bool {
	_, err := os.Stat(filename)
	return !os.IsNotExist(err)
}

// 根据文件扩展名设置合理的音频 Content-Type
func detectAudioContentType(name string) string {
	switch strings.ToLower(filepath.Ext(name)) {
	case ".mp3":
		return "audio/mpeg"
	case ".flac":
		return "audio/flac"
	case ".ogg":
		return "audio/ogg"
	case ".wav":
		return "audio/wav"
	case ".m4a", ".aac":
		return "audio/aac"
	default:
		return "application/octet-stream"
	}
}

// Route handlers
func rootHandler(w http.ResponseWriter, r *http.Request) {
	enableCORS(w)
	fmt.Fprintln(w, "Welcome to the Go Music Server!")
}

// File serving handlers
func serveAudioFileHandler(w http.ResponseWriter, r *http.Request) {
	enableCORS(w)

	// Extract folder and filename from URL path
	pathParts := strings.Split(strings.Trim(r.URL.Path, "/"), "/")
	if len(pathParts) < 3 {
		http.Error(w, "Invalid path", http.StatusBadRequest)
		return
	}

	// 处理 URL 编码（例如中文路径）
	rawFolder := pathParts[1]
	rawFilename := pathParts[2]
	folder, errDec1 := url.PathUnescape(rawFolder)
	filename, errDec2 := url.PathUnescape(rawFilename)
	if errDec1 != nil {
		folder = rawFolder
	}
	if errDec2 != nil {
		filename = rawFilename
	}

	filePath := filepath.Join(UPLOAD_FOLDER, folder, filename)

	fi, err := os.Stat(filePath)
	if err != nil || fi.IsDir() {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusNotFound)
		_ = json.NewEncoder(w).Encode(map[string]string{"error": "File not found"})
		return
	}

	f, err := os.Open(filePath)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	rs := io.NewSectionReader(f, 0, fi.Size())

	// 重要：防止中间层变换（如 gzip/转码）导致字节数与帧边界破坏
	w.Header().Set("Cache-Control", "public, no-transform")
	w.Header().Set("Connection", "keep-alive")
	w.Header().Set("Content-Type", detectAudioContentType(filename))
	w.Header().Set("Accept-Ranges", "bytes")
	w.Header().Set("Content-Disposition", "inline; filename="+filename)

	http.ServeContent(w, r, filename, fi.ModTime(), rs)
	_ = f.Close()
}

func serveLrcHandler(w http.ResponseWriter, r *http.Request) {
	enableCORS(w)

	// Extract folder and filename from URL path
	pathParts := strings.Split(strings.Trim(r.URL.Path, "/"), "/")
	if len(pathParts) < 4 {
		http.Error(w, "Invalid path", http.StatusBadRequest)
		return
	}

	folder := pathParts[1]
	filename := pathParts[2]

	lrcPath := filepath.Join(UPLOAD_FOLDER, folder, filename)

	if !fileExists(lrcPath) {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusNotFound)
		json.NewEncoder(w).Encode(map[string]string{"error": "File not found"})
		return
	}

	file, err := os.Open(lrcPath)
	if err != nil {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusInternalServerError)
		json.NewEncoder(w).Encode(map[string]string{"error": err.Error()})
		return
	}
	defer file.Close()

	content, err := io.ReadAll(file)
	if err != nil {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusInternalServerError)
		json.NewEncoder(w).Encode(map[string]string{"error": err.Error()})
		return
	}

	lines := strings.Split(string(content), "\n")
	var cleanLines []string
	for _, line := range lines {
		cleanLines = append(cleanLines, strings.TrimSpace(line))
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(cleanLines)
}

// Stream handler
func streamHandler(w http.ResponseWriter, r *http.Request) {
	enableCORS(w)

	if r.Method == "OPTIONS" {
		w.WriteHeader(http.StatusOK)
		return
	}

	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var req StreamRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusBadRequest)
		json.NewEncoder(w).Encode(map[string]string{"error": "Missing filename"})
		return
	}

	filename := strings.TrimSpace(req.Filename)
	baseName := strings.TrimSuffix(filename, filepath.Ext(filename))
	folderPath := filepath.Join(UPLOAD_FOLDER, baseName)

	if !fileExists(folderPath) {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusNotFound)
		json.NewEncoder(w).Encode(map[string]string{"error": "Folder not found"})
		return
	}

	files, err := os.ReadDir(folderPath)
	if err != nil {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusInternalServerError)
		json.NewEncoder(w).Encode(map[string]string{"error": err.Error()})
		return
	}

	var audioFiles, lrcFiles, albumCovers []string

	for _, file := range files {
		if file.IsDir() {
			continue
		}

		ext := strings.ToLower(filepath.Ext(file.Name()))
		switch ext {
		case ".flac", ".mp3", ".ogg":
			audioFiles = append(audioFiles, file.Name())
		case ".lrc":
			lrcFiles = append(lrcFiles, file.Name())
		case ".png":
			albumCovers = append(albumCovers, file.Name())
		}
	}

	if len(audioFiles) == 0 {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusNotFound)
		json.NewEncoder(w).Encode(map[string]string{"error": "No audio files"})
		return
	}

	audioPath := filepath.Join(folderPath, audioFiles[0])
	songID, _ := generateSongID(audioPath)
	duration, _ := getAudioDuration(audioPath)

	response := StreamResponse{
		Message:   "Audio stream started successfully",
		StreamURL: fmt.Sprintf("http://%s/uploads/%s/%s", r.Host, baseName, audioFiles[0]),
		ID:        songID,
		Duration:  duration,
	}

	if len(lrcFiles) > 0 {
		lrcURL := fmt.Sprintf("http://%s/uploads/%s/%s", r.Host, baseName, lrcFiles[0])
		response.LrcURL = &lrcURL
	}

	if len(albumCovers) > 0 {
		coverURL := fmt.Sprintf("http://%s/uploads/%s/%s", r.Host, baseName, albumCovers[0])
		response.AlbumCoverURL = &coverURL
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(response)
}

// File listing handlers
func listFilesHandler(w http.ResponseWriter, r *http.Request) {
	enableCORS(w)

	// Try cache first
	cacheKey := CACHE_PREFIX_FILES + "all"
	if cachedResult, err := rdb.Get(ctx, cacheKey).Result(); err == nil {
		w.Header().Set("Content-Type", "application/json")
		w.Write([]byte(cachedResult))
		return
	}

	result := make(map[string]string)

	err := filepath.Walk(UPLOAD_FOLDER, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return nil
		}

		if !info.IsDir() {
			ext := strings.ToLower(filepath.Ext(info.Name()))
			if ext == ".flac" || ext == ".mp3" || ext == ".ogg" {
				duration, err := getAudioDuration(path)
				if err != nil {
					result[info.Name()] = "Error"
				} else {
					result[info.Name()] = fmt.Sprintf("%.2f seconds", *duration)
				}
			}
		}
		return nil
	})

	if err != nil {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusInternalServerError)
		json.NewEncoder(w).Encode(map[string]string{"error": err.Error()})
		return
	}

	// Cache result
	resultJSON, _ := json.Marshal(result)
	rdb.Set(ctx, cacheKey, resultJSON, CACHE_TTL_MEDIUM)

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(result)
}

func listFilesWithNameHandler(w http.ResponseWriter, r *http.Request) {
	enableCORS(w)

	if r.Method == "OPTIONS" {
		w.WriteHeader(http.StatusOK)
		return
	}

	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var req FileSearchRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusBadRequest)
		json.NewEncoder(w).Encode(map[string]string{"error": "Missing filename"})
		return
	}

	nameFilter := strings.ToLower(strings.TrimSpace(req.Filename))
	result := make(map[string]string)

	err := filepath.Walk(UPLOAD_FOLDER, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return nil
		}

		if info.IsDir() {
			folderName := strings.ToLower(filepath.Base(path))
			if strings.Contains(folderName, nameFilter) {
				// List files in this folder
				files, err := os.ReadDir(path)
				if err != nil {
					return nil
				}

				for _, file := range files {
					if file.IsDir() {
						continue
					}

					ext := strings.ToLower(filepath.Ext(file.Name()))
					if ext == ".flac" || ext == ".mp3" || ext == ".ogg" {
						fullPath := filepath.Join(path, file.Name())
						relPath, _ := filepath.Rel(UPLOAD_FOLDER, fullPath)
						duration, err := getAudioDuration(fullPath)
						if err != nil {
							result[relPath] = "Error"
						} else {
							result[relPath] = fmt.Sprintf("%.2f seconds", *duration)
						}
					}
				}
			}
		}
		return nil
	})

	if err != nil {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusInternalServerError)
		json.NewEncoder(w).Encode(map[string]string{"error": err.Error()})
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(result)
}

// File upload handler
func uploadHandler(w http.ResponseWriter, r *http.Request) {
	enableCORS(w)

	if r.Method == "OPTIONS" {
		w.WriteHeader(http.StatusOK)
		return
	}

	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	r.ParseMultipartForm(int64(MAX_FILE_SIZE))

	file, handler, err := r.FormFile("file")
	if err != nil {
		http.Error(w, "No file part", http.StatusBadRequest)
		return
	}
	defer file.Close()

	if handler.Filename == "" {
		http.Error(w, "No selected file", http.StatusBadRequest)
		return
	}

	filePath := filepath.Join(UPLOAD_FOLDER, handler.Filename)

	dst, err := os.Create(filePath)
	if err != nil {
		http.Error(w, "Error creating file: "+err.Error(), http.StatusInternalServerError)
		return
	}
	defer dst.Close()

	if _, err := io.Copy(dst, file); err != nil {
		http.Error(w, "Error saving file: "+err.Error(), http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusOK)
	fmt.Fprintln(w, "File uploaded successfully!")
}

// File download handler
func downloadHandler(w http.ResponseWriter, r *http.Request) {
	enableCORS(w)

	if r.Method == "OPTIONS" {
		w.WriteHeader(http.StatusOK)
		return
	}

	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var req FileSearchRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusBadRequest)
		json.NewEncoder(w).Encode(map[string]string{"error": "Missing filename"})
		return
	}

	filename := req.Filename
	folderName := strings.TrimSuffix(filename, filepath.Ext(filename))
	filePath := filepath.Join(UPLOAD_FOLDER, folderName, filename)

	if !fileExists(filePath) {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusNotFound)
		json.NewEncoder(w).Encode(map[string]string{"error": "File not found"})
		return
	}

	w.Header().Set("Content-Disposition", "attachment; filename="+filename)
	http.ServeFile(w, r, filePath)
}

// User management handlers
func registerHandler(w http.ResponseWriter, r *http.Request) {
	enableCORS(w)

	if r.Method == "OPTIONS" {
		w.WriteHeader(http.StatusOK)
		return
	}

	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var req UserRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Missing or invalid fields", http.StatusBadRequest)
		return
	}

	cPassword := C.CString(req.Password)
	cAccount := C.CString(req.Account)
	cUsername := C.CString(req.Username)
	defer C.free(unsafe.Pointer(cPassword))
	defer C.free(unsafe.Pointer(cAccount))
	defer C.free(unsafe.Pointer(cUsername))

	result := C.user_register_c(cPassword, cAccount, cUsername)

	w.Header().Set("Content-Type", "application/json")
	success := result == 1
	json.NewEncoder(w).Encode(map[string]bool{"success": success})
}

func loginHandler(w http.ResponseWriter, r *http.Request) {
	enableCORS(w)

	if r.Method == "OPTIONS" {
		w.WriteHeader(http.StatusOK)
		return
	}

	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var req UserRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Missing or invalid fields", http.StatusBadRequest)
		return
	}

	// Check cache first
	cacheKey := CACHE_PREFIX_USER + "login:" + req.Account + ":" + req.Password
	if cachedResult, err := rdb.Get(ctx, cacheKey).Result(); err == nil {
		w.Header().Set("Content-Type", "application/json")
		w.Write([]byte(cachedResult))
		return
	}

	cPassword := C.CString(req.Password)
	cAccount := C.CString(req.Account)
	defer C.free(unsafe.Pointer(cPassword))
	defer C.free(unsafe.Pointer(cAccount))

	resultPtr := C.user_login_c(cPassword, cAccount)
	defer C.free(unsafe.Pointer(resultPtr))

	resultJSON := C.GoString(resultPtr)

	// Cache successful login for short period
	if strings.Contains(resultJSON, "success") {
		rdb.Set(ctx, cacheKey, resultJSON, CACHE_TTL_SHORT)
	}

	w.Header().Set("Content-Type", "application/json")
	w.Write([]byte(resultJSON))
}

func addMusicHandler(w http.ResponseWriter, r *http.Request) {
	enableCORS(w)

	if r.Method == "OPTIONS" {
		w.WriteHeader(http.StatusOK)
		return
	}

	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var req AddMusicRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Missing or invalid fields", http.StatusBadRequest)
		return
	}

	cUsername := C.CString(req.Username)
	cMusicPath := C.CString(req.MusicPath)
	defer C.free(unsafe.Pointer(cUsername))
	defer C.free(unsafe.Pointer(cMusicPath))

	result := C.insert_music_c(cUsername, cMusicPath)

	w.Header().Set("Content-Type", "application/json")
	success := result == 1
	json.NewEncoder(w).Encode(map[string]bool{"success": success})
}

// Legacy handlers for compatibility
func getRecordsHandler(w http.ResponseWriter, r *http.Request) {
	enableCORS(w)

	var result *C.char = C.get_records()
	defer C.free(unsafe.Pointer(result))

	recordsJSON := C.GoString(result)

	w.Header().Set("Content-Type", "application/json")
	w.Write([]byte(recordsJSON))
}

func addRecordHandler(w http.ResponseWriter, r *http.Request) {
	enableCORS(w)

	if r.Method != http.MethodPost {
		http.Error(w, "Invalid request method", http.StatusMethodNotAllowed)
		return
	}

	type Record struct {
		ID    int    `json:"id"`
		Name  string `json:"name"`
		Value string `json:"value"`
	}

	var record Record
	err := json.NewDecoder(r.Body).Decode(&record)
	if err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	cName := C.CString(record.Name)
	cValue := C.CString(record.Value)
	defer C.free(unsafe.Pointer(cName))
	defer C.free(unsafe.Pointer(cValue))

	C.add_record(cName, cValue)

	w.WriteHeader(http.StatusCreated)
	fmt.Fprintln(w, "Record added successfully")
}

func main() {
	// 解析命令行参数
	configPath := flag.String("config", "config.yaml", "配置文件路径")
	flag.Parse()

	// 读取配置文件
	f, err := os.Open(*configPath)
	if err != nil {
		log.Fatalf("无法打开配置文件: %v", err)
	}
	defer f.Close()
	decoder := yaml.NewDecoder(f)
	if err := decoder.Decode(&config); err != nil {
		log.Fatalf("解析配置文件失败: %v", err)
	}

	// 设置全局目录变量
	UPLOAD_FOLDER = config.Server.UploadDir
	STATIC_FOLDER = config.Server.StaticDir

	// 初始化Redis
	initRedis()
	defer rdb.Close()

	// 确保上传目录存在
	if err := os.MkdirAll(UPLOAD_FOLDER, 0755); err != nil {
		log.Fatal("Failed to create upload directory:", err)
	}

	// 预热缓存
	go func() {
		log.Println("Pre-warming cache...")
		time.Sleep(2 * time.Second)
		req, _ := http.NewRequest("GET", fmt.Sprintf("http://localhost:%d/files", config.Server.Port), nil)
		resp := &http.Response{}
		listFilesHandler(&responseWriter{header: make(http.Header)}, req)
		if resp != nil {
			log.Println("Cache pre-warmed successfully")
		}
	}()

	// 配置HTTP服务
	addr := fmt.Sprintf("%s:%d", config.Server.Host, config.Server.Port)
	server := &http.Server{
		Addr:         addr,
		ReadTimeout:  0,                // 不限制读取超时（适合大文件/慢客户端）
		WriteTimeout: 0,                // 不限制写入超时，防止长流被截断
		IdleTimeout:  10 * time.Minute, // 允许空闲长连
	}

	// File serving routes
	http.HandleFunc("/uploads/", func(w http.ResponseWriter, r *http.Request) {
		if strings.HasSuffix(r.URL.Path, "/lrc") {
			serveLrcHandler(w, r)
		} else {
			serveAudioFileHandler(w, r)
		}
	})

	// Main API routes
	http.HandleFunc("/", rootHandler)
	http.HandleFunc("/stream", streamHandler)
	http.HandleFunc("/files", listFilesHandler)
	http.HandleFunc("/file", listFilesWithNameHandler)
	http.HandleFunc("/upload", uploadHandler)
	http.HandleFunc("/download", downloadHandler)

	// User management routes
	http.HandleFunc("/users/register", registerHandler)
	http.HandleFunc("/users/login", loginHandler)
	http.HandleFunc("/users/add_music", addMusicHandler)

	// Legacy routes for compatibility
	http.HandleFunc("/records", getRecordsHandler)
	http.HandleFunc("/add", addRecordHandler)

	// Health check endpoint
	http.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		enableCORS(w)

		// Check Redis connection
		_, redisErr := rdb.Ping(ctx).Result()

		status := map[string]interface{}{
			"status":    "ok",
			"timestamp": time.Now().Unix(),
			"redis":     redisErr == nil,
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(status)
	})

	// Statistics endpoint
	http.HandleFunc("/stats", func(w http.ResponseWriter, r *http.Request) {
		enableCORS(w)

		// Get Redis stats
		info := rdb.Info(ctx, "stats").Val()

		stats := map[string]interface{}{
			"redis_info": info,
			"timestamp":  time.Now().Unix(),
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(stats)
	})

	fmt.Printf("Starting Go Music Server with Redis on %s...\n", addr)
	fmt.Println("Available endpoints:")
	fmt.Println("  GET  /                     - Welcome message")
	fmt.Println("  GET  /uploads/<folder>/<file> - Serve audio files")
	fmt.Println("  GET  /uploads/<folder>/<file>/lrc - Serve lyric files")
	fmt.Println("  POST /stream               - Start audio stream")
	fmt.Println("  GET  /files                - List all audio files (cached)")
	fmt.Println("  POST /file                 - Search audio files by name")
	fmt.Println("  POST /upload               - Upload files")
	fmt.Println("  POST /download             - Download files")
	fmt.Println("  POST /users/register       - User registration")
	fmt.Println("  POST /users/login          - User login (cached)")
	fmt.Println("  POST /users/add_music      - Add music to user library")
	fmt.Println("  GET  /records              - Get database records (legacy)")
	fmt.Println("  POST /add                  - Add record (legacy)")
	fmt.Println("  GET  /health               - Health check")
	fmt.Println("  GET  /stats                - Redis statistics")
	fmt.Printf("Redis: %s\n", "Connected")

	log.Fatal(server.ListenAndServe())
}

// Helper type for pre-warming cache
type responseWriter struct {
	header http.Header
	body   []byte
	status int
}

func (rw *responseWriter) Header() http.Header {
	return rw.header
}

func (rw *responseWriter) Write(data []byte) (int, error) {
	rw.body = append(rw.body, data...)
	return len(data), nil
}

func (rw *responseWriter) WriteHeader(statusCode int) {
	rw.status = statusCode
}
