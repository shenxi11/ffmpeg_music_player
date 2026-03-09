-- MySQL dump 10.13  Distrib 8.0.45, for Linux (x86_64)
--
-- Host: 127.0.0.1    Database: music_users
-- ------------------------------------------------------
-- Server version	8.0.45-0ubuntu0.24.04.1

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Current Database: `music_users`
--

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `music_users` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci */ /*!80016 DEFAULT ENCRYPTION='N' */;

USE `music_users`;

--
-- Dumping data for table `artists`
--

LOCK TABLES `artists` WRITE;
/*!40000 ALTER TABLE `artists` DISABLE KEYS */;
INSERT INTO `artists` (`id`, `name`, `created_at`, `updated_at`) VALUES (1,'周杰伦','2026-02-03 03:53:24','2026-02-03 03:53:24');
INSERT INTO `artists` (`id`, `name`, `created_at`, `updated_at`) VALUES (2,'方大同','2026-02-03 03:53:24','2026-02-03 03:53:24');
INSERT INTO `artists` (`id`, `name`, `created_at`, `updated_at`) VALUES (3,'张真源','2026-02-03 03:53:24','2026-02-03 03:53:24');
INSERT INTO `artists` (`id`, `name`, `created_at`, `updated_at`) VALUES (4,'林俊杰','2026-02-03 03:53:24','2026-02-03 03:53:24');
INSERT INTO `artists` (`id`, `name`, `created_at`, `updated_at`) VALUES (8,'Gentle Bones','2026-02-03 03:53:24','2026-02-03 03:53:24');
INSERT INTO `artists` (`id`, `name`, `created_at`, `updated_at`) VALUES (9,'Luis Fonsi','2026-02-03 03:53:24','2026-02-03 03:53:24');
INSERT INTO `artists` (`id`, `name`, `created_at`, `updated_at`) VALUES (10,'五月天','2026-02-03 03:53:24','2026-02-03 03:53:24');
INSERT INTO `artists` (`id`, `name`, `created_at`, `updated_at`) VALUES (11,'周华健','2026-02-03 03:53:24','2026-02-03 03:53:24');
INSERT INTO `artists` (`id`, `name`, `created_at`, `updated_at`) VALUES (12,'周深','2026-02-03 03:53:24','2026-02-03 03:53:24');
INSERT INTO `artists` (`id`, `name`, `created_at`, `updated_at`) VALUES (13,'孙燕姿','2026-02-03 03:53:24','2026-02-03 03:53:24');
INSERT INTO `artists` (`id`, `name`, `created_at`, `updated_at`) VALUES (14,'张杰','2026-02-03 03:53:24','2026-02-03 03:53:24');
INSERT INTO `artists` (`id`, `name`, `created_at`, `updated_at`) VALUES (15,'徐佳莹','2026-02-03 03:53:24','2026-02-03 03:53:24');
INSERT INTO `artists` (`id`, `name`, `created_at`, `updated_at`) VALUES (23,'费玉清','2026-02-03 03:53:24','2026-02-03 03:53:24');
INSERT INTO `artists` (`id`, `name`, `created_at`, `updated_at`) VALUES (24,'张明敏','2026-02-03 03:53:24','2026-02-03 03:53:24');
INSERT INTO `artists` (`id`, `name`, `created_at`, `updated_at`) VALUES (25,'张韶涵','2026-02-03 03:53:24','2026-02-03 03:53:24');
/*!40000 ALTER TABLE `artists` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `domain_event_dlq`
--

LOCK TABLES `domain_event_dlq` WRITE;
/*!40000 ALTER TABLE `domain_event_dlq` DISABLE KEYS */;
/*!40000 ALTER TABLE `domain_event_dlq` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `domain_events`
--

LOCK TABLES `domain_events` WRITE;
/*!40000 ALTER TABLE `domain_events` DISABLE KEYS */;
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (1,'user.favorite.added-1772107599164','user.favorite.added','profile-service','{\"is_local\": false, \"music_path\": \"http://slcdut.xyz:8080/uploads/千里之外/千里之外.mp3\", \"user_account\": \"root\"}','2026-02-26 12:06:39');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (2,'user.play_history.added-1772107599215','user.play_history.added','profile-service','{\"is_local\": false, \"music_path\": \"http://slcdut.xyz:8080/uploads/千里之外/千里之外.mp3\", \"user_account\": \"root\"}','2026-02-26 12:06:39');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (3,'user.play_history.added-1772107772540','user.play_history.added','profile-service','{\"is_local\": false, \"music_path\": \"http://slcdut.xyz:8080/uploads/花海/花海.mp3\", \"user_account\": \"root\"}','2026-02-26 12:09:32');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (4,'user.play_history.added-1772107979864467265-1-985485','user.play_history.added','profile-service','{\"is_local\": false, \"music_path\": \"http://slcdut.xyz:8080/uploads/test/test.mp3\", \"user_account\": \"root\"}','2026-02-26 12:12:59');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (5,'manual-outbox-1772107993512275791','user.favorite.added','profile-service','{\"music_path\": \"manual.mp3\", \"user_account\": \"root\"}','2026-02-26 12:13:14');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (6,'user.play_history.added-1772112291682560195-1-70475','user.play_history.added','profile-service','{\"is_local\": false, \"music_path\": \"http://slcdut.xyz:8080/uploads/stream-check/stream-check.mp3\", \"user_account\": \"root\"}','2026-02-26 13:24:51');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (7,'user.play_history.added-1772113161382853748-1-38899','user.play_history.added','profile-service','{\"is_local\": false, \"music_path\": \"http://slcdut.xyz:8080/uploads/pending-test-1772113161351965157/pending-test-1772113161351965157.mp3\", \"user_account\": \"root\"}','2026-02-26 13:39:51');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (8,'user.favorite.added-1772113599581733767-1-864074','user.favorite.added','profile-service','{\"is_local\": false, \"music_path\": \"http://slcdut.xyz:8080/uploads/schema-split-1772113599520337514/schema-split-1772113599520337514.mp3\", \"user_account\": \"root\"}','2026-02-26 13:46:39');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (9,'user.favorite.added-1772115150812817410-1-769582','user.favorite.added','profile-service','{\"is_local\": false, \"music_path\": \"http://127.0.0.1:8080/uploads/test/1772115150350225251.mp3\", \"user_account\": \"api_u_1772115150350225251\"}','2026-02-26 14:12:30');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (10,'user.favorite.removed-1772115150859009223-2-433222','user.favorite.removed','profile-service','{\"music_path\": \"http://127.0.0.1:8080/uploads/test/1772115150350225251.mp3\", \"user_account\": \"api_u_1772115150350225251\"}','2026-02-26 14:12:30');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (11,'user.play_history.added-1772115150888028317-3-602564','user.play_history.added','profile-service','{\"is_local\": false, \"music_path\": \"http://127.0.0.1:8080/uploads/h/1772115150350225251.mp3\", \"user_account\": \"api_u_1772115150350225251\"}','2026-02-26 14:12:30');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (12,'user.play_history.deleted-1772115150945981725-4-931855','user.play_history.deleted','profile-service','{\"count\": 1, \"user_account\": \"api_u_1772115150350225251\"}','2026-02-26 14:12:30');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (13,'user.play_history.cleared-1772115150960310863-5-569088','user.play_history.cleared','profile-service','{\"count\": 0, \"user_account\": \"api_u_1772115150350225251\"}','2026-02-26 14:12:30');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (14,'user.favorite.added-1772115338118342001-6-487679','user.favorite.added','profile-service','{\"is_local\": false, \"music_path\": \"http://127.0.0.1:8080/uploads/test/1772115337833212526.mp3\", \"user_account\": \"api_u_1772115337833212526\"}','2026-02-26 14:15:38');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (15,'user.favorite.removed-1772115338162486292-7-617383','user.favorite.removed','profile-service','{\"music_path\": \"http://127.0.0.1:8080/uploads/test/1772115337833212526.mp3\", \"user_account\": \"api_u_1772115337833212526\"}','2026-02-26 14:15:38');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (16,'user.play_history.added-1772115338182643719-8-898528','user.play_history.added','profile-service','{\"is_local\": false, \"music_path\": \"http://127.0.0.1:8080/uploads/h/1772115337833212526.mp3\", \"user_account\": \"api_u_1772115337833212526\"}','2026-02-26 14:15:38');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (17,'user.play_history.deleted-1772115338222284312-9-582106','user.play_history.deleted','profile-service','{\"count\": 1, \"user_account\": \"api_u_1772115337833212526\"}','2026-02-26 14:15:38');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (18,'user.play_history.cleared-1772115338248965771-10-552480','user.play_history.cleared','profile-service','{\"count\": 0, \"user_account\": \"api_u_1772115337833212526\"}','2026-02-26 14:15:38');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (19,'user.favorite.added-1772115677580187706-1-429351','user.favorite.added','profile-service','{\"is_local\": false, \"music_path\": \"http://127.0.0.1:8080/uploads/test/1772115677003893249.mp3\", \"user_account\": \"doc_u_1772115677003893249\"}','2026-02-26 14:21:17');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (20,'user.favorite.removed-1772115677638919310-2-58308','user.favorite.removed','profile-service','{\"music_path\": \"http://127.0.0.1:8080/uploads/test/1772115677003893249.mp3\", \"user_account\": \"doc_u_1772115677003893249\"}','2026-02-26 14:21:17');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (21,'user.play_history.added-1772115677661247074-3-106725','user.play_history.added','profile-service','{\"is_local\": false, \"music_path\": \"http://127.0.0.1:8080/uploads/history/1772115677003893249.mp3\", \"user_account\": \"doc_u_1772115677003893249\"}','2026-02-26 14:21:17');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (22,'user.play_history.deleted-1772115677701820280-4-175744','user.play_history.deleted','profile-service','{\"count\": 1, \"user_account\": \"doc_u_1772115677003893249\"}','2026-02-26 14:21:17');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (23,'user.play_history.cleared-1772115677731059267-5-717044','user.play_history.cleared','profile-service','{\"count\": 0, \"user_account\": \"doc_u_1772115677003893249\"}','2026-02-26 14:21:17');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (24,'user.favorite.added-1772115739137362510-6-766633','user.favorite.added','profile-service','{\"is_local\": false, \"music_path\": \"http://127.0.0.1:8080/uploads/test/1772115738736798283.mp3\", \"user_account\": \"doc_u_1772115738736798283\"}','2026-02-26 14:22:19');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (25,'user.favorite.removed-1772115739186437057-7-668548','user.favorite.removed','profile-service','{\"music_path\": \"http://127.0.0.1:8080/uploads/test/1772115738736798283.mp3\", \"user_account\": \"doc_u_1772115738736798283\"}','2026-02-26 14:22:19');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (26,'user.play_history.added-1772115739209064129-8-552792','user.play_history.added','profile-service','{\"is_local\": false, \"music_path\": \"http://127.0.0.1:8080/uploads/history/1772115738736798283.mp3\", \"user_account\": \"doc_u_1772115738736798283\"}','2026-02-26 14:22:19');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (27,'user.play_history.deleted-1772115739247318867-9-963578','user.play_history.deleted','profile-service','{\"count\": 1, \"user_account\": \"doc_u_1772115738736798283\"}','2026-02-26 14:22:19');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (28,'user.play_history.cleared-1772115739273088003-10-106713','user.play_history.cleared','profile-service','{\"count\": 0, \"user_account\": \"doc_u_1772115738736798283\"}','2026-02-26 14:22:19');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (29,'user.play_history.deleted-1772118307215891489-11-802237','user.play_history.deleted','profile-service','{\"count\": 1, \"user_account\": \"root\"}','2026-02-26 15:05:07');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (30,'user.play_history.deleted-1772118308712386300-12-135330','user.play_history.deleted','profile-service','{\"count\": 1, \"user_account\": \"root\"}','2026-02-26 15:05:08');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (31,'user.play_history.deleted-1772118309767900559-13-245376','user.play_history.deleted','profile-service','{\"count\": 1, \"user_account\": \"root\"}','2026-02-26 15:05:09');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (32,'user.favorite.removed-1772118313588212083-14-960211','user.favorite.removed','profile-service','{\"music_path\": \"http://slcdut.xyz:8080/uploads/schema-split-1772113599520337514/schema-split-1772113599520337514.mp3\", \"user_account\": \"root\"}','2026-02-26 15:05:13');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (33,'user.play_history.added-1772118316980235225-15-610364','user.play_history.added','profile-service','{\"is_local\": false, \"music_path\": \"http://slcdut.xyz:8080/uploads/千里之外/千里之外.mp3\", \"user_account\": \"root\"}','2026-02-26 15:05:16');
INSERT INTO `domain_events` (`id`, `event_id`, `event_type`, `event_source`, `payload`, `created_at`) VALUES (34,'user.play_history.added-1772118322188740004-16-908869','user.play_history.added','profile-service','{\"is_local\": false, \"music_path\": \"http://slcdut.xyz:8080/uploads/花海/花海.mp3\", \"user_account\": \"root\"}','2026-02-26 15:05:22');
/*!40000 ALTER TABLE `domain_events` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `event_outbox`
--

LOCK TABLES `event_outbox` WRITE;
/*!40000 ALTER TABLE `event_outbox` DISABLE KEYS */;
INSERT INTO `event_outbox` (`id`, `event_id`, `event_type`, `event_source`, `event_version`, `event_timestamp`, `payload`, `status`, `retry_count`, `next_retry_at`, `last_error`, `created_at`, `updated_at`, `published_at`) VALUES (1,'manual-outbox-1772107993512275791','user.favorite.added','profile-service',1,1772107993512,'{\"music_path\": \"manual.mp3\", \"user_account\": \"root\"}',1,0,'2026-02-26 12:13:13','manual_inject','2026-02-26 12:13:13','2026-02-26 12:13:14','2026-02-26 12:13:14');
/*!40000 ALTER TABLE `event_outbox` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `music_files`
--

LOCK TABLES `music_files` WRITE;
/*!40000 ALTER TABLE `music_files` DISABLE KEYS */;
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (1,'hua_hai/hua_hai.mp3','hua_hai','','',264.672653,4239403,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','hua_hai/hua_hai.lrc','');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (2,'千里之外/千里之外.mp3','千里之外','周杰伦&费玉清','依然范特西',254.275918,10191609,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','千里之外/千里之外.lrc','千里之外/千里之外_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (3,'彩虹/彩虹.mp3','彩虹','周杰伦','我很忙',261.955918,10509676,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','彩虹/彩虹.lrc','彩虹/彩虹_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (4,'告白气球/告白气球.mp3','告白气球','周杰伦','周杰伦的床边故事',215.640816,8652236,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','告白气球/告白气球.lrc','告白气球/告白气球_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (5,'周杰伦 - 七里香_EM/周杰伦 - 七里香_EM.flac','周杰伦 - 七里香_EM','','',299.200115,224822472,'.flac',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','周杰伦 - 七里香_EM/周杰伦 - 七里香_EM.lrc','周杰伦 - 七里香_EM/周杰伦 - 七里香_EM_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (6,'枫/枫.mp3','枫','周杰伦','十一月的萧邦',277.054694,11099358,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','枫/枫.lrc','枫/枫_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (7,'稻香/稻香.mp3','稻香','周杰伦','魔杰座',223.503673,8965406,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','稻香/稻香.lrc','稻香/稻香_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (8,'费玉清-千里之外/费玉清-千里之外.mp3','千里之外','周杰伦&费玉清','依然范特西',254.275918,10191609,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','','费玉清-千里之外/费玉清-千里之外_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (9,'发如雪/发如雪.mp3','发如雪','周杰伦','十一月的萧邦',299.284898,11989176,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','发如雪/发如雪.lrc','发如雪/发如雪_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (10,'安静/安静.mp3','安静','周杰伦','范特西',334.262857,13389337,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','安静/安静.lrc','安静/安静_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (11,'花海/花海.mp3','花海','周杰伦','魔杰座',264.646531,10610857,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','花海/花海.lrc','花海/花海_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (12,'本草纲目/本草纲目.mp3','本草纲目','周杰伦','依然范特西',206.889796,8296442,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','本草纲目/本草纲目.lrc','本草纲目/本草纲目_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (13,'白色风车/白色风车.mp3','白色风车','周杰伦','依然范特西',270.236735,10829943,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','白色风车/白色风车.lrc','白色风车/白色风车_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (14,'给我一首歌的时间/给我一首歌的时间.mp3','给我一首歌的时间','周杰伦','魔杰座',253.596735,10169693,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','给我一首歌的时间/给我一首歌的时间.lrc','给我一首歌的时间/给我一首歌的时间_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (15,'东风破/东风破.mp3','东风破','周杰伦','叶惠美',315.454694,12642551,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','东风破/东风破.lrc','东风破/东风破_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (16,'搁浅/搁浅.mp3','搁浅','周杰伦','七里香',238.18449,9554760,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','搁浅/搁浅.lrc','搁浅/搁浅_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (17,'断了的弦/断了的弦.mp3','断了的弦','周杰伦','寻找周杰伦',297.560816,11925336,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','断了的弦/断了的弦.lrc','断了的弦/断了的弦_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (18,'BEYOND - 海阔天空/BEYOND - 海阔天空.ogg','BEYOND - 海阔天空','','',239.491678,8766641,'.ogg',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','BEYOND - 海阔天空/BEYOND - 海阔天空.lrc','');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (19,'Love_Song/Love_Song.mp3','Love Song(Live)','方大同','15 香港演唱会(2011Live)',357.407347,14350087,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','Love_Song/Love_Song.lrc','Love_Song/Love_Song_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (21,'床边故事/床边故事.mp3','床边故事','周杰伦','周杰伦的床边故事',226.246531,9077422,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','床边故事/床边故事.lrc','床边故事/床边故事_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (22,'暗号/暗号.mp3','暗号','周杰伦','八度空间',271.804082,10891105,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','暗号/暗号.lrc','暗号/暗号_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (23,'爱在西元前/爱在西元前.mp3','爱在西元前','周杰伦','范特西',234.292245,9390361,'.mp3',1,'2025-12-01 16:27:35','2025-12-01 16:27:35','爱在西元前/爱在西元前.lrc','爱在西元前/爱在西元前_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (24,'等你下课 (with 杨瑞代)/等你下课 (with 杨瑞代).mp3','等你下课 (with 杨瑞代)','周杰伦','最伟大的作品',270.027755,10820157,'.mp3',1,'2025-12-01 16:27:36','2025-12-01 16:27:36','','等你下课 (with 杨瑞代)/等你下课 (with 杨瑞代)_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (25,'yi_lu_xiang_bei/yi_lu_xiang_bei.mp3','yi_lu_xiang_bei','','',293.3839,2463289,'.mp3',1,'2025-12-01 16:27:36','2025-12-01 16:27:36','yi_lu_xiang_bei/yi_lu_xiang_bei.lrc','');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (26,'半兽人/半兽人.mp3','半兽人','周杰伦','八度空间',247.431837,9916635,'.mp3',1,'2025-12-01 16:27:36','2025-12-01 16:27:36','半兽人/半兽人.lrc','半兽人/半兽人_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (27,'爱情废柴/爱情废柴.mp3','爱情废柴','周杰伦','周杰伦的床边故事',285.64898,11452800,'.mp3',1,'2025-12-01 16:27:36','2025-12-01 16:27:36','爱情废柴/爱情废柴.lrc','爱情废柴/爱情废柴_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (28,'红尘客栈/红尘客栈.mp3','红尘客栈','周杰伦','十二新作',275.644082,11048722,'.mp3',1,'2025-12-01 16:27:36','2025-12-01 16:27:36','红尘客栈/红尘客栈.lrc','红尘客栈/红尘客栈_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (29,'qi_li_xiang/qi_li_xiang.ogg','qi_li_xiang','','',299.200113,11993357,'.ogg',1,'2025-12-01 16:27:36','2025-12-01 16:27:36','qi_li_xiang/qi_li_xiang.lrc','');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (30,'不爱我就拉倒/不爱我就拉倒.mp3','不爱我就拉倒','周杰伦','不爱我就拉倒',245.890612,9856968,'.mp3',1,'2025-12-01 16:27:36','2025-12-01 16:27:36','不爱我就拉倒/不爱我就拉倒.lrc','不爱我就拉倒/不爱我就拉倒_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (31,'轨迹/轨迹.mp3','轨迹','周杰伦','寻找周杰伦',326.896327,13098457,'.mp3',1,'2025-12-01 16:27:36','2025-12-01 16:27:36','轨迹/轨迹.lrc','轨迹/轨迹_cover.png');
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (32,'Gentle Bones&林俊杰-At Least I Had You/Gentle Bones&林俊杰-At Least I Had You.mp3','At Least I Had You','Gentle Bones&林俊杰','At Least I Had You',203.00625850340137,8124200,'mp3',1,'2026-02-03 02:12:29','2026-02-03 02:12:29','Gentle Bones&林俊杰-At Least I Had You/Gentle Bones&林俊杰-At Least I Had You.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (33,'Luis Fonsi&林俊杰-Despacito 缓缓 (Mandarin Version)/Luis Fonsi&林俊杰-Despacito 缓缓 (Mandarin Version).mp3','Despacito 缓缓 (Mandarin Version)','Luis Fonsi&林俊杰','Despacito 缓缓 (Mandarin Version)',229.33333333333334,9177546,'mp3',1,'2026-02-03 02:12:29','2026-02-03 02:12:29','Luis Fonsi&林俊杰-Despacito 缓缓 (Mandarin Version)/Luis Fonsi&林俊杰-Despacito 缓缓 (Mandarin Version).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (34,'五月天&林俊杰-不为谁而作的歌/五月天&林俊杰-不为谁而作的歌.mp3','不为谁而作的歌','五月天&林俊杰','',256.99950113378685,4114251,'mp3',1,'2026-02-03 02:12:29','2026-02-03 02:12:29','五月天&林俊杰-不为谁而作的歌/五月天&林俊杰-不为谁而作的歌.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (35,'五月天&林俊杰-知足 (2024「回到那一天」25周年巡回演唱会上海站) (现场)/五月天&林俊杰-知足 (2024「回到那一天」25周年巡回演唱会上海站) (现场).mp3','知足 (2024「回到那一天」25周年巡回演唱会上海站) (现场)','五月天&林俊杰','',138.20798185941044,2213425,'mp3',1,'2026-02-03 02:12:29','2026-02-03 02:12:29','五月天&林俊杰-知足 (2024「回到那一天」25周年巡回演唱会上海站) (现场)/五月天&林俊杰-知足 (2024「回到那一天」25周年巡回演唱会上海站) (现场).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (36,'周华健&谢霆锋&容祖儿&林俊杰&张明敏-明天会更好 (2008中央电视台爱的奉献抗震救灾募捐晚会现场)/周华健&谢霆锋&容祖儿&林俊杰&张明敏-明天会更好 (2008中央电视台爱的奉献抗震救灾募捐晚会现场).mp3','明天会更好 (2008中央电视台爱的奉献抗震救灾募捐晚会现场)','周华健&谢霆锋&容祖儿&林俊杰&张明敏','',197.4595918367347,3161376,'mp3',1,'2026-02-03 02:12:29','2026-02-03 02:12:29','周华健&谢霆锋&容祖儿&林俊杰&张明敏-明天会更好 (2008中央电视台爱的奉献抗震救灾募捐晚会现场)/周华健&谢霆锋&容祖儿&林俊杰&张明敏-明天会更好 (2008中央电视台爱的奉献抗震救灾募捐晚会现场).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (37,'周杰伦&林俊杰-Stay With You (50秒Live片段)/周杰伦&林俊杰-Stay With You (50秒Live片段).mp3','Stay With You (50秒Live片段)','周杰伦&林俊杰','',50.083990929705216,803632,'mp3',1,'2026-02-03 02:12:30','2026-02-03 02:12:30','周杰伦&林俊杰-Stay With You (50秒Live片段)/周杰伦&林俊杰-Stay With You (50秒Live片段).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (38,'周杰伦&林俊杰-修炼爱情+最长的电影 (2017地表最强演唱会台北站)/周杰伦&林俊杰-修炼爱情+最长的电影 (2017地表最强演唱会台北站).mp3','修炼爱情+最长的电影 (2017地表最强演唱会台北站)','周杰伦&林俊杰','',200.6204081632653,3211917,'mp3',1,'2026-02-03 02:12:30','2026-02-03 02:12:30','周杰伦&林俊杰-修炼爱情+最长的电影 (2017地表最强演唱会台北站)/周杰伦&林俊杰-修炼爱情+最长的电影 (2017地表最强演唱会台北站).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (39,'周杰伦&林俊杰-晴天 (2017周杰伦地表最强演唱会台北站)/周杰伦&林俊杰-晴天 (2017周杰伦地表最强演唱会台北站).mp3','晴天 (2017周杰伦地表最强演唱会台北站)','周杰伦&林俊杰','',239.22938775510204,3829233,'mp3',1,'2026-02-03 02:12:30','2026-02-03 02:12:30','周杰伦&林俊杰-晴天 (2017周杰伦地表最强演唱会台北站)/周杰伦&林俊杰-晴天 (2017周杰伦地表最强演唱会台北站).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (40,'周杰伦&林俊杰-晴天+安静+修炼爱情+最长的电影 (2017周杰伦地表最强演唱会台北站)/周杰伦&林俊杰-晴天+安静+修炼爱情+最长的电影 (2017周杰伦地表最强演唱会台北站).mp3','晴天+安静+修炼爱情+最长的电影 (2017周杰伦地表最强演唱会台北站)','周杰伦&林俊杰','',268.25142857142856,4293613,'mp3',1,'2026-02-03 02:12:30','2026-02-03 02:12:30','周杰伦&林俊杰-晴天+安静+修炼爱情+最长的电影 (2017周杰伦地表最强演唱会台北站)/周杰伦&林俊杰-晴天+安静+修炼爱情+最长的电影 (2017周杰伦地表最强演唱会台北站).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (41,'周杰伦&林俊杰-算什么男人 (Live)/周杰伦&林俊杰-算什么男人 (Live).mp3','算什么男人 (Live)','周杰伦&林俊杰','',285.51943310657595,11424994,'mp3',1,'2026-02-03 02:12:30','2026-02-03 02:12:30','周杰伦&林俊杰-算什么男人 (Live)/周杰伦&林俊杰-算什么男人 (Live).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (42,'周深&林俊杰-大鱼 (Live)/周深&林俊杰-大鱼 (Live).mp3','大鱼 (Live)','周深&林俊杰','',196.53650793650795,3146677,'mp3',1,'2026-02-03 02:12:30','2026-02-03 02:12:30','周深&林俊杰-大鱼 (Live)/周深&林俊杰-大鱼 (Live).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (43,'周深&林俊杰-裹着心的光 (Live)/周深&林俊杰-裹着心的光 (Live).mp3','裹着心的光 (Live)','周深&林俊杰','',189.05968253968254,3027147,'mp3',1,'2026-02-03 02:12:30','2026-02-03 02:12:30','周深&林俊杰-裹着心的光 (Live)/周深&林俊杰-裹着心的光 (Live).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (44,'孙燕姿&林俊杰-我怀念的 (Live)/孙燕姿&林俊杰-我怀念的 (Live).mp3','我怀念的 (Live)','孙燕姿&林俊杰','',307.3160997732426,4919248,'mp3',1,'2026-02-03 02:12:30','2026-02-03 02:12:30','孙燕姿&林俊杰-我怀念的 (Live)/孙燕姿&林俊杰-我怀念的 (Live).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (45,'张杰&林俊杰-最美的太阳 + 翅膀 (Live)/张杰&林俊杰-最美的太阳 + 翅膀 (Live).mp3','最美的太阳 + 翅膀 (Live)','张杰&林俊杰','我是歌手第二季 总决赛',271.8684126984127,10878555,'mp3',1,'2026-02-03 02:12:30','2026-02-03 02:12:30','张杰&林俊杰-最美的太阳 + 翅膀 (Live)/张杰&林俊杰-最美的太阳 + 翅膀 (Live).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (46,'张真源-背对背拥抱 (片段)/张真源-背对背拥抱 (片段).mp3','背对背拥抱 (片段)','张真源','',21.05469387755102,338824,'mp3',1,'2026-02-03 02:12:30','2026-02-03 02:12:30','张真源-背对背拥抱 (片段)/张真源-背对背拥抱 (片段).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (47,'徐佳莹&林俊杰-不为谁而作的歌 (Live)/徐佳莹&林俊杰-不为谁而作的歌 (Live).mp3','不为谁而作的歌 (Live)','徐佳莹&林俊杰','我是歌手第四季 歌王之战',268.62167800453517,10748633,'mp3',1,'2026-02-03 02:12:31','2026-02-03 02:12:31','徐佳莹&林俊杰-不为谁而作的歌 (Live)/徐佳莹&林俊杰-不为谁而作的歌 (Live).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (48,'方大同&薛凯琪-四人遊/方大同&薛凯琪-四人遊.dsf','方大同&薛凯琪-四人遊','','',228.29333333333332,161106446,'dsf',1,'2026-02-03 02:12:31','2026-02-03 02:12:31','方大同&薛凯琪-四人遊/方大同&薛凯琪-四人遊.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (49,'方大同-Goodbye Melody Rose/方大同-Goodbye Melody Rose.dsf','方大同-Goodbye Melody Rose','','',186.99990929705214,131967502,'dsf',1,'2026-02-03 02:12:31','2026-02-03 02:12:31','方大同-Goodbye Melody Rose/方大同-Goodbye Melody Rose.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (50,'方大同-If You Leave Me Now/方大同-If You Leave Me Now.dsf','方大同-If You Leave Me Now','','',222.98666666666668,157362702,'dsf',1,'2026-02-03 02:12:31','2026-02-03 02:12:31','方大同-If You Leave Me Now/方大同-If You Leave Me Now.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (51,'方大同-Love Outrolude(Instrumental)/方大同-Love Outrolude(Instrumental).dsf','方大同-Love Outrolude(Instrumental)','','',92,64940558,'dsf',1,'2026-02-03 02:12:31','2026-02-03 02:12:31','方大同-Love Outrolude(Instrumental)/方大同-Love Outrolude(Instrumental).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (52,'方大同-偷笑/方大同-偷笑.dsf','方大同-偷笑','','',227.01333333333332,160205326,'dsf',1,'2026-02-03 02:12:31','2026-02-03 02:12:31','方大同-偷笑/方大同-偷笑.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (53,'方大同-唉!/方大同-唉!.dsf','方大同-唉!','','',220.70657596371882,155757070,'dsf',1,'2026-02-03 02:12:31','2026-02-03 02:12:31','方大同-唉!/方大同-唉!.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (54,'方大同-手拖手/方大同-手拖手.dsf','方大同-手拖手','','',163.79990929705215,115599886,'dsf',1,'2026-02-03 02:12:31','2026-02-03 02:12:31','方大同-手拖手/方大同-手拖手.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (55,'方大同-拖男带女/方大同-拖男带女.dsf','方大同-拖男带女','','',294.4932426303855,207817230,'dsf',1,'2026-02-03 02:12:31','2026-02-03 02:12:31','方大同-拖男带女/方大同-拖男带女.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (56,'方大同-春风吹之吹吹风mix(吹吹风Mix)/方大同-春风吹之吹吹风mix(吹吹风Mix).dsf','方大同-春风吹之吹吹风mix(吹吹风Mix)','','',204.1732426303855,144091662,'dsf',1,'2026-02-03 02:12:31','2026-02-03 02:12:31','方大同-春风吹之吹吹风mix(吹吹风Mix)/方大同-春风吹之吹吹风mix(吹吹风Mix).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (57,'方大同-歌手与模特儿/方大同-歌手与模特儿.dsf','方大同-歌手与模特儿','','',221.70666666666668,156461582,'dsf',1,'2026-02-03 02:12:31','2026-02-03 02:12:31','方大同-歌手与模特儿/方大同-歌手与模特儿.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (58,'方大同-爱爱爱/方大同-爱爱爱.dsf','方大同-爱爱爱','','',213.26666666666668,150505998,'dsf',1,'2026-02-03 02:12:31','2026-02-03 02:12:31','方大同-爱爱爱/方大同-爱爱爱.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (59,'方大同-苏丽珍/方大同-苏丽珍.dsf','方大同-苏丽珍','','',217.09333333333333,153201166,'dsf',1,'2026-02-03 02:12:32','2026-02-03 02:12:32','方大同-苏丽珍/方大同-苏丽珍.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (60,'方大同-诗人的情人/方大同-诗人的情人.dsf','方大同-诗人的情人','','',231.50657596371883,163375630,'dsf',1,'2026-02-03 02:12:32','2026-02-03 02:12:32','方大同-诗人的情人/方大同-诗人的情人.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (61,'林俊杰&张韶涵-保护色/林俊杰&张韶涵-保护色.flac','保护色','林俊杰&张韶涵','她说 概念自选辑',199.28,23460419,'flac',1,'2026-02-03 02:12:32','2026-02-03 02:12:32','林俊杰&张韶涵-保护色/林俊杰&张韶涵-保护色.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (62,'林俊杰-Down(Demo)/林俊杰-Down(Demo).flac','Down(Demo)','林俊杰','曹操',165,14152957,'flac',1,'2026-02-03 02:12:32','2026-02-03 02:12:32','林俊杰-Down(Demo)/林俊杰-Down(Demo).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (63,'林俊杰-I AM/林俊杰-I AM.flac','I AM','林俊杰','她说 概念自选辑',296.5187074829932,32508541,'flac',1,'2026-02-03 02:12:32','2026-02-03 02:12:32','林俊杰-I AM/林俊杰-I AM.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (64,'林俊杰-Now That She\'s Gone/林俊杰-Now That She\'s Gone.flac','Now That She\'s Gone','林俊杰','曹操',269,30066036,'flac',1,'2026-02-03 02:12:32','2026-02-03 02:12:32','林俊杰-Now That She\'s Gone/林俊杰-Now That She\'s Gone.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (65,'林俊杰-一个又一个/林俊杰-一个又一个.mp3','一个又一个','林俊杰','100天',264.28,10574446,'mp3',1,'2026-02-03 02:12:32','2026-02-03 02:12:32','林俊杰-一个又一个/林俊杰-一个又一个.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (66,'林俊杰-一千年以前/林俊杰-一千年以前.mp3','一千年以前','林俊杰','编号89757',64,2563219,'mp3',1,'2026-02-03 02:12:32','2026-02-03 02:12:32','林俊杰-一千年以前/林俊杰-一千年以前.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (67,'林俊杰-一千年以后/林俊杰-一千年以后.mp3','一千年以后','林俊杰','编号89757',227.65714285714284,9108460,'mp3',1,'2026-02-03 02:12:32','2026-02-03 02:12:32','林俊杰-一千年以后/林俊杰-一千年以后.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (68,'林俊杰-一千年以后 & 学不会 山东卫视2013新年演唱会 现场版 12、12、31/林俊杰-一千年以后 & 学不会 山东卫视2013新年演唱会 现场版 12、12、31.mp3','一千年以后 & 学不会 山东卫视2013新年演唱会 现场版 12/12/31','林俊杰','',220.0565759637188,3523311,'mp3',1,'2026-02-03 02:12:32','2026-02-03 02:12:32','林俊杰-一千年以后 & 学不会 山东卫视2013新年演唱会 现场版 12、12、31/林俊杰-一千年以后 & 学不会 山东卫视2013新年演唱会 现场版 12、12、31.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (69,'林俊杰-一千年以后 (Live)/林俊杰-一千年以后 (Live).mp3','一千年以后 (Live)','林俊杰','谁是大歌神 第3期',47,1883578,'mp3',1,'2026-02-03 02:12:32','2026-02-03 02:12:32','林俊杰-一千年以后 (Live)/林俊杰-一千年以后 (Live).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (70,'林俊杰-一千年后，记得我/林俊杰-一千年后，记得我.mp3','一千年后，记得我','林俊杰','因你 而在',257.1333333333333,10289197,'mp3',1,'2026-02-03 02:12:32','2026-02-03 02:12:32','林俊杰-一千年后，记得我/林俊杰-一千年后，记得我.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (71,'林俊杰-一定会/林俊杰-一定会.mp3','一定会','林俊杰','一定会 / After The Rain',207.2532426303855,8293462,'mp3',1,'2026-02-03 02:12:33','2026-02-03 02:12:33','林俊杰-一定会/林俊杰-一定会.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (72,'林俊杰-一时的选择/林俊杰-一时的选择.mp3','一时的选择','林俊杰','重拾_快乐',237.4997052154195,9503428,'mp3',1,'2026-02-03 02:12:33','2026-02-03 02:12:33','林俊杰-一时的选择/林俊杰-一时的选择.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (73,'林俊杰-一时的选择 (Live)/林俊杰-一时的选择 (Live).mp3','一时的选择 (Live)','林俊杰','',134.5364172335601,2154860,'mp3',1,'2026-02-03 02:12:33','2026-02-03 02:12:33','林俊杰-一时的选择 (Live)/林俊杰-一时的选择 (Live).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (74,'林俊杰-一生的爱/林俊杰-一生的爱.flac','一生的爱','林俊杰','她说 概念自选辑',213.72,23957410,'flac',1,'2026-02-03 02:12:33','2026-02-03 02:12:33','林俊杰-一生的爱/林俊杰-一生的爱.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (75,'林俊杰-一眼万年/林俊杰-一眼万年.flac','一眼万年','林俊杰','她说 概念自选辑',258.08,30277884,'flac',1,'2026-02-03 02:12:33','2026-02-03 02:12:33','林俊杰-一眼万年/林俊杰-一眼万年.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (76,'林俊杰-一路向北/林俊杰-一路向北.mp3','一路向北','林俊杰','',112.01160997732427,1794145,'mp3',1,'2026-02-03 02:12:33','2026-02-03 02:12:33','林俊杰-一路向北/林俊杰-一路向北.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (77,'林俊杰-不死之身/林俊杰-不死之身.flac','不死之身','林俊杰','曹操',227,25395596,'flac',1,'2026-02-03 02:12:33','2026-02-03 02:12:33','林俊杰-不死之身/林俊杰-不死之身.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (78,'林俊杰-为什么相爱的人不能在一起/林俊杰-为什么相爱的人不能在一起.mp3','为什么相爱的人不能在一起','林俊杰','',36.02285714285714,578318,'mp3',1,'2026-02-03 02:12:33','2026-02-03 02:12:33','林俊杰-为什么相爱的人不能在一起/林俊杰-为什么相爱的人不能在一起.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (79,'林俊杰-主角/林俊杰-主角.mp3','主角','林俊杰','爱情睡醒了 电视剧原声带',212.37333333333333,8498244,'mp3',1,'2026-02-03 02:12:33','2026-02-03 02:12:33','林俊杰-主角/林俊杰-主角.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (80,'林俊杰-以后要做的事/林俊杰-以后要做的事.mp3','以后要做的事','林俊杰','因你 而在',294.14666666666665,11769813,'mp3',1,'2026-02-03 02:12:33','2026-02-03 02:12:33','林俊杰-以后要做的事/林俊杰-以后要做的事.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (81,'林俊杰-伟大的渺小/林俊杰-伟大的渺小.mp3','伟大的渺小','林俊杰','伟大的渺小',277.62666666666667,11108391,'mp3',1,'2026-02-03 02:12:33','2026-02-03 02:12:33','林俊杰-伟大的渺小/林俊杰-伟大的渺小.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (82,'林俊杰-伟大的渺小 (Jazz Version)/林俊杰-伟大的渺小 (Jazz Version).mp3','伟大的渺小 (Jazz Version)','林俊杰','感爵这一刻',298.7733333333333,11954788,'mp3',1,'2026-02-03 02:12:34','2026-02-03 02:12:34','林俊杰-伟大的渺小 (Jazz Version)/林俊杰-伟大的渺小 (Jazz Version).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (83,'林俊杰-你要的不是我/林俊杰-你要的不是我.flac','你要的不是我','林俊杰','曹操',251,23530517,'flac',1,'2026-02-03 02:12:34','2026-02-03 02:12:34','林俊杰-你要的不是我/林俊杰-你要的不是我.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (84,'林俊杰-修炼爱情/林俊杰-修炼爱情.mp3','修炼爱情','林俊杰','因你 而在',287,11483507,'mp3',1,'2026-02-03 02:12:34','2026-02-03 02:12:34','林俊杰-修炼爱情/林俊杰-修炼爱情.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (85,'林俊杰-修炼爱情 (2013 Hito流行音乐奖颁奖典礼现场)/林俊杰-修炼爱情 (2013 Hito流行音乐奖颁奖典礼现场).mp3','修炼爱情 (2013 Hito流行音乐奖颁奖典礼现场)','林俊杰','',155.06285714285715,6204706,'mp3',1,'2026-02-03 02:12:34','2026-02-03 02:12:34','林俊杰-修炼爱情 (2013 Hito流行音乐奖颁奖典礼现场)/林俊杰-修炼爱情 (2013 Hito流行音乐奖颁奖典礼现场).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (86,'林俊杰-修炼爱情 (2015台北跨年演唱会)/林俊杰-修炼爱情 (2015台北跨年演唱会).mp3','修炼爱情 (2015台北跨年演唱会)','林俊杰','',271.25551020408165,10852394,'mp3',1,'2026-02-03 02:12:34','2026-02-03 02:12:34','林俊杰-修炼爱情 (2015台北跨年演唱会)/林俊杰-修炼爱情 (2015台北跨年演唱会).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (87,'林俊杰-修炼爱情 (Instrumental Version)/林俊杰-修炼爱情 (Instrumental Version).mp3','修炼爱情 (Instrumental Version)','林俊杰','因你而在',296.04571428571427,4738317,'mp3',1,'2026-02-03 02:12:34','2026-02-03 02:12:34','林俊杰-修炼爱情 (Instrumental Version)/林俊杰-修炼爱情 (Instrumental Version).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (88,'林俊杰-修炼爱情 (Jazz Version)/林俊杰-修炼爱情 (Jazz Version).mp3','修炼爱情 (Jazz Version)','林俊杰','感爵这一刻',331.62503401360544,13269268,'mp3',1,'2026-02-03 02:12:34','2026-02-03 02:12:34','林俊杰-修炼爱情 (Jazz Version)/林俊杰-修炼爱情 (Jazz Version).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (89,'林俊杰-修炼爱情 (Live)/林俊杰-修炼爱情 (Live).mp3','修炼爱情 (Live)','林俊杰','2014QQ音乐巅峰榜暨年度盛典',186.12,7448147,'mp3',1,'2026-02-03 02:12:34','2026-02-03 02:12:34','林俊杰-修炼爱情 (Live)/林俊杰-修炼爱情 (Live).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (90,'林俊杰-修炼爱情 + 黑夜问白天 + 交换余生 + 那些你很冒险的梦 (2023 bilibili毕业歌会现场)/林俊杰-修炼爱情 + 黑夜问白天 + 交换余生 + 那些你很冒险的梦 (2023 bilibili毕业歌会现场).mp3','修炼爱情 + 黑夜问白天 + 交换余生 + 那些你很冒险的梦 (2023 bilibili毕业歌会现场)','林俊杰','',623.2826984126984,9974957,'mp3',1,'2026-02-03 02:12:34','2026-02-03 02:12:34','林俊杰-修炼爱情 + 黑夜问白天 + 交换余生 + 那些你很冒险的梦 (2023 bilibili毕业歌会现场)/林俊杰-修炼爱情 + 黑夜问白天 + 交换余生 + 那些你很冒险的梦 (2023 bilibili毕业歌会现场).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (91,'林俊杰-像我这样的人 (28秒2018梦想的声音第三季第9期现场片段)/林俊杰-像我这样的人 (28秒2018梦想的声音第三季第9期现场片段).mp3','像我这样的人 (28秒2018梦想的声音第三季第9期现场片段)','林俊杰','',28.490884353741496,457984,'mp3',1,'2026-02-03 02:12:34','2026-02-03 02:12:34','林俊杰-像我这样的人 (28秒2018梦想的声音第三季第9期现场片段)/林俊杰-像我这样的人 (28秒2018梦想的声音第三季第9期现场片段).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (92,'林俊杰-压力/林俊杰-压力.mp3','压力','林俊杰','乐行者',190.92045351473922,7640365,'mp3',1,'2026-02-03 02:12:34','2026-02-03 02:12:34','林俊杰-压力/林俊杰-压力.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (93,'林俊杰-原来/林俊杰-原来.flac','原来','林俊杰','曹操',220.96,25054723,'flac',1,'2026-02-03 02:12:35','2026-02-03 02:12:35','林俊杰-原来/林俊杰-原来.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (94,'林俊杰-只对你有感觉/林俊杰-只对你有感觉.flac','只对你有感觉','林俊杰','她说 概念自选辑',266.94666666666666,35252672,'flac',1,'2026-02-03 02:12:35','2026-02-03 02:12:35','林俊杰-只对你有感觉/林俊杰-只对你有感觉.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (95,'林俊杰-只对你说 (Live)/林俊杰-只对你说 (Live).mp3','只对你说 (Live)','林俊杰','',227.98666666666668,9123074,'mp3',1,'2026-02-03 02:12:35','2026-02-03 02:12:35','林俊杰-只对你说 (Live)/林俊杰-只对你说 (Live).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (96,'林俊杰-只要有你的地方/林俊杰-只要有你的地方.mp3','只要有你的地方','林俊杰','和自己对话 From M.E. To Myself',291.56281179138324,11666411,'mp3',1,'2026-02-03 02:12:35','2026-02-03 02:12:35','林俊杰-只要有你的地方/林俊杰-只要有你的地方.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (97,'林俊杰-只要有你的地方 (晚安版)/林俊杰-只要有你的地方 (晚安版).mp3','只要有你的地方 (晚安版)','林俊杰','和自己对话 From M.E. To Myself',276.9066666666667,11080235,'mp3',1,'2026-02-03 02:12:35','2026-02-03 02:12:35','林俊杰-只要有你的地方 (晚安版)/林俊杰-只要有你的地方 (晚安版).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (98,'林俊杰-因你而在/林俊杰-因你而在.mp3','因你而在','林俊杰','因你 而在',265.73333333333335,10632960,'mp3',1,'2026-02-03 02:12:35','2026-02-03 02:12:35','林俊杰-因你而在/林俊杰-因你而在.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (99,'林俊杰-她说/林俊杰-她说.flac','她说','林俊杰','她说 概念自选辑',320.53333333333336,31302528,'flac',1,'2026-02-03 02:12:35','2026-02-03 02:12:35','林俊杰-她说/林俊杰-她说.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (100,'林俊杰-子弹列车/林俊杰-子弹列车.mp3','子弹列车','林俊杰','第二天堂',204,8163865,'mp3',1,'2026-02-03 02:12:35','2026-02-03 02:12:35','林俊杰-子弹列车/林俊杰-子弹列车.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (101,'林俊杰-学不会/林俊杰-学不会.mp3','学不会','林俊杰','学不会',229.50009070294786,9183681,'mp3',1,'2026-02-03 02:12:35','2026-02-03 02:12:35','林俊杰-学不会/林俊杰-学不会.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (102,'林俊杰-学不会(伴奏版)/林俊杰-学不会(伴奏版).mp3','学不会(伴奏版)','林俊杰','',234.91918367346938,1880880,'mp3',1,'2026-02-03 02:12:35','2026-02-03 02:12:35','林俊杰-学不会(伴奏版)/林俊杰-学不会(伴奏版).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (103,'林俊杰-完美新世界/林俊杰-完美新世界.flac','完美新世界','林俊杰','她说 概念自选辑',277.4266666666667,30972693,'flac',1,'2026-02-03 02:12:35','2026-02-03 02:12:35','林俊杰-完美新世界/林俊杰-完美新世界.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (104,'林俊杰-小瓶子/林俊杰-小瓶子.mp3','小瓶子','林俊杰','伟大的渺小',252.62666666666667,10108420,'mp3',1,'2026-02-03 02:12:35','2026-02-03 02:12:35','林俊杰-小瓶子/林俊杰-小瓶子.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (105,'林俊杰-小酒窝/林俊杰-小酒窝.mp3','小酒窝','林俊杰','他是... JJ林俊杰',217.70666666666668,8712448,'mp3',1,'2026-02-03 02:12:35','2026-02-03 02:12:35','林俊杰-小酒窝/林俊杰-小酒窝.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (106,'林俊杰-小酒窝 (2023马栏山芒果节不设限毕业礼现场) (Live)/林俊杰-小酒窝 (2023马栏山芒果节不设限毕业礼现场) (Live).mp3','小酒窝 (2023马栏山芒果节不设限毕业礼现场) (Live)','林俊杰','',197.5,3162180,'mp3',1,'2026-02-03 02:12:35','2026-02-03 02:12:35','林俊杰-小酒窝 (2023马栏山芒果节不设限毕业礼现场) (Live)/林俊杰-小酒窝 (2023马栏山芒果节不设限毕业礼现场) (Live).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (107,'林俊杰-小酒窝 (Live)/林俊杰-小酒窝 (Live).mp3','小酒窝 (Live)','林俊杰','I AM 世界巡回演唱会',300.0003628117914,12003893,'mp3',1,'2026-02-03 02:12:36','2026-02-03 02:12:36','林俊杰-小酒窝 (Live)/林俊杰-小酒窝 (Live).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (108,'林俊杰-小酒窝 (胡彦斌 男人 ktv 江南) (其他)/林俊杰-小酒窝 (胡彦斌 男人 ktv 江南) (其他).mp3','小酒窝 (胡彦斌 男人 ktv 江南) (其他)','林俊杰','',273.5553514739229,4379263,'mp3',1,'2026-02-03 02:12:36','2026-02-03 02:12:36','林俊杰-小酒窝 (胡彦斌 男人 ktv 江南) (其他)/林俊杰-小酒窝 (胡彦斌 男人 ktv 江南) (其他).lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (109,'林俊杰-幸存者/林俊杰-幸存者.mp3','幸存者','林俊杰','幸存者 Drifter',281.9103401360544,11279762,'mp3',1,'2026-02-03 02:12:36','2026-02-03 02:12:36','林俊杰-幸存者/林俊杰-幸存者.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (110,'林俊杰-序曲：12年前/林俊杰-序曲：12年前.mp3','序曲：12年前','林俊杰','和自己对话 From M.E. To Myself',367.1333333333333,14688899,'mp3',1,'2026-02-03 02:12:36','2026-02-03 02:12:36',NULL,NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (111,'林俊杰-当你/林俊杰-当你.flac','当你','林俊杰','她说 概念自选辑',251.29333333333332,28610660,'flac',1,'2026-02-03 02:12:36','2026-02-03 02:12:36','林俊杰-当你/林俊杰-当你.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (112,'林俊杰-心墙/林俊杰-心墙.flac','心墙','林俊杰','她说 概念自选辑',225.61333333333334,25459096,'flac',1,'2026-02-03 02:12:36','2026-02-03 02:12:36','林俊杰-心墙/林俊杰-心墙.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (113,'林俊杰-我很想爱他/林俊杰-我很想爱他.flac','我很想爱他','林俊杰','她说 概念自选辑',261.3333333333333,27080065,'flac',1,'2026-02-03 02:12:36','2026-02-03 02:12:36','林俊杰-我很想爱他/林俊杰-我很想爱他.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (114,'林俊杰-握不住的他/林俊杰-握不住的他.flac','握不住的他','林俊杰','她说 概念自选辑',211.21333333333334,23380347,'flac',1,'2026-02-03 02:12:36','2026-02-03 02:12:36','林俊杰-握不住的他/林俊杰-握不住的他.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (115,'林俊杰-曹操/林俊杰-曹操.flac','曹操','林俊杰','曹操',242,29372876,'flac',1,'2026-02-03 02:12:36','2026-02-03 02:12:36','林俊杰-曹操/林俊杰-曹操.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (116,'林俊杰-波间带/林俊杰-波间带.flac','波间带','林俊杰','曹操',210,26105789,'flac',1,'2026-02-03 02:12:36','2026-02-03 02:12:36','林俊杰-波间带/林俊杰-波间带.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (117,'林俊杰-流行主教/林俊杰-流行主教.flac','流行主教','林俊杰','曹操',128.89333333333335,16325279,'flac',1,'2026-02-03 02:12:36','2026-02-03 02:12:36','林俊杰-流行主教/林俊杰-流行主教.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (118,'林俊杰-熟能生巧/林俊杰-熟能生巧.flac','熟能生巧','林俊杰','曹操',246.98666666666668,28663171,'flac',1,'2026-02-03 02:12:36','2026-02-03 02:12:36','林俊杰-熟能生巧/林俊杰-熟能生巧.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (119,'林俊杰-爱情Yogurt/林俊杰-爱情Yogurt.flac','爱情Yogurt','林俊杰','曹操',225,26849956,'flac',1,'2026-02-03 02:12:37','2026-02-03 02:12:37','林俊杰-爱情Yogurt/林俊杰-爱情Yogurt.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (120,'林俊杰-爱笑的眼睛/林俊杰-爱笑的眼睛.flac','爱笑的眼睛','林俊杰','她说 概念自选辑',253.57333333333332,25742428,'flac',1,'2026-02-03 02:12:37','2026-02-03 02:12:37','林俊杰-爱笑的眼睛/林俊杰-爱笑的眼睛.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (121,'林俊杰-真材实料的我/林俊杰-真材实料的我.flac','真材实料的我','林俊杰','她说 概念自选辑',215.49333333333334,29640377,'flac',1,'2026-02-03 02:12:37','2026-02-03 02:12:37','林俊杰-真材实料的我/林俊杰-真材实料的我.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (122,'林俊杰-记得/林俊杰-记得.flac','记得','林俊杰','她说 概念自选辑',287.84,30235695,'flac',1,'2026-02-03 02:12:37','2026-02-03 02:12:37','林俊杰-记得/林俊杰-记得.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (123,'林俊杰-进化论/林俊杰-进化论.flac','进化论','林俊杰','曹操',258,31197758,'flac',1,'2026-02-03 02:12:37','2026-02-03 02:12:37','林俊杰-进化论/林俊杰-进化论.lrc',NULL);
INSERT INTO `music_files` (`id`, `path`, `title`, `artist`, `album`, `duration_sec`, `size_bytes`, `file_type`, `is_audio`, `created_at`, `updated_at`, `lrc_path`, `cover_art_path`) VALUES (124,'林俊杰-사랑해요只对你说/林俊杰-사랑해요只对你说.flac','사랑해요只对你说','林俊杰','曹操',227.98666666666668,23734672,'flac',1,'2026-02-03 02:12:37','2026-02-03 02:12:37','林俊杰-사랑해요只对你说/林俊杰-사랑해요只对你说.lrc',NULL);
/*!40000 ALTER TABLE `music_files` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `schema_migrations`
--

LOCK TABLES `schema_migrations` WRITE;
/*!40000 ALTER TABLE `schema_migrations` DISABLE KEYS */;
INSERT INTO `schema_migrations` (`version`, `checksum`, `applied_at`) VALUES ('20260226_catalog_legacy_user_path_expand.sql','3bd17b125752a0ce9143c2ab07eb69fb6c90650f3f8c3f3154f810b3f40a5b8f','2026-02-26 14:15:15');
INSERT INTO `schema_migrations` (`version`, `checksum`, `applied_at`) VALUES ('20260226_event_reliability.sql','7e38259e79acdd4364d8bc28e00dbecb1b94e942568b961f8f0404e78f8abc26','2026-02-26 13:57:39');
/*!40000 ALTER TABLE `schema_migrations` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `user_favorite_music`
--

LOCK TABLES `user_favorite_music` WRITE;
/*!40000 ALTER TABLE `user_favorite_music` DISABLE KEYS */;
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (5,'gotest123','/sdcard/Music/林俊杰 - 江南.mp3','江南','林俊杰',260,1,'2026-02-04 09:22:23');
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (17,'gotest123','千里之外/千里之外.mp3','千里之外','周杰伦/费玉清',269,0,'2026-02-04 13:54:04');
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (18,'gotest123','告白气球/告白气球.mp3','告白气球','周杰伦',203,0,'2026-02-04 13:54:04');
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (20,'root','白色风车/白色风车.mp3','白色风车.mp3','周杰伦',270,0,'2026-02-04 14:28:28');
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (21,'root','给我一首歌的时间/给我一首歌的时间.mp3','给我一首歌的时间.mp3','周杰伦',253,0,'2026-02-04 14:28:32');
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (22,'root','E:/MP3/爱情废柴/爱情废柴.mp3','爱情废柴.mp3','未知艺术家',285,1,'2026-02-26 06:16:23');
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (32,'root','轨迹/轨迹.mp3','轨迹.mp3','周杰伦',326,0,'2026-02-26 09:12:00');
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (35,'root','http://slcdut.xyz:8080/uploads/床边故事/床边故事.mp3','床边故事','周杰伦',226,0,'2026-02-26 10:28:01');
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (36,'root','花海/花海.mp3','花海.mp3','周杰伦',264,0,'2026-02-26 10:38:38');
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (37,'root','http://slcdut.xyz:8080/uploads/千里之外/千里之外.mp3','千里之外','周杰伦&费玉清',254.2,0,'2026-02-26 12:06:39');
/*!40000 ALTER TABLE `user_favorite_music` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `user_path`
--

LOCK TABLES `user_path` WRITE;
/*!40000 ALTER TABLE `user_path` DISABLE KEYS */;
INSERT INTO `user_path` (`id`, `username`, `music_path`) VALUES (1,'SL','/home/shen/shared/BEYOND - 海阔天空.ogg');
INSERT INTO `user_path` (`id`, `username`, `music_path`) VALUES (5,'SL','/home/shen/shared/ai_qing_fei_chai.mp3');
INSERT INTO `user_path` (`id`, `username`, `music_path`) VALUES (6,'SL','/home/shen/shared/周杰伦 - 七里香_EM.flac');
INSERT INTO `user_path` (`id`, `username`, `music_path`) VALUES (7,'SL','/home/shen/shared/yi_lu_xiang_bei.mp3');
INSERT INTO `user_path` (`id`, `username`, `music_path`) VALUES (8,'SL','/home/shen/shared/ye_qu.mp3');
INSERT INTO `user_path` (`id`, `username`, `music_path`) VALUES (11,'SL','/home/shen/shared/Love_Song.mp3');
INSERT INTO `user_path` (`id`, `username`, `music_path`) VALUES (12,'Test User','BEYOND - 海阔天空.ogg');
INSERT INTO `user_path` (`id`, `username`, `music_path`) VALUES (14,'shenLiang','E:/MP3/爱情废柴/爱情废柴.mp3');
INSERT INTO `user_path` (`id`, `username`, `music_path`) VALUES (15,'SL','E:/MP3/暗号/暗号.mp3');
INSERT INTO `user_path` (`id`, `username`, `music_path`) VALUES (30,'测试用户','告白气球/告白气球.mp3');
INSERT INTO `user_path` (`id`, `username`, `music_path`) VALUES (31,'gotest','hua_hai/hua_hai.mp3');
INSERT INTO `user_path` (`id`, `username`, `music_path`) VALUES (32,'gotest','hua_hai/hua_hai.mp3');
INSERT INTO `user_path` (`id`, `username`, `music_path`) VALUES (33,'gotest','林俊杰-修炼爱情 + 黑夜问白天 + 交换余生 + 那些你很冒险的梦 (2023 bilibili毕业歌会现场)/林俊杰-修炼爱情 + 黑夜问白天 + 交换余生 + 那些你很冒险的梦 (2023 bilibili毕业歌会现场).mp3');
INSERT INTO `user_path` (`id`, `username`, `music_path`) VALUES (34,'api_user_1772115337833212526','http://127.0.0.1:8080/uploads/test/1772115337833212526.mp3');
INSERT INTO `user_path` (`id`, `username`, `music_path`) VALUES (35,'doc_user_1772115677003893249','hua_hai/hua_hai.mp3');
INSERT INTO `user_path` (`id`, `username`, `music_path`) VALUES (36,'doc_user_1772115738736798283','hua_hai/hua_hai.mp3');
/*!40000 ALTER TABLE `user_path` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `user_play_history`
--

LOCK TABLES `user_play_history` WRITE;
/*!40000 ALTER TABLE `user_play_history` DISABLE KEYS */;
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (5,'gotest123','/uploads/music/周杰伦 - 七里香.mp3','七里香','周杰伦','七里香',300,0,'2026-02-04 09:22:24');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (6,'gotest123','/uploads/music/周杰伦 - 七里香.mp3','七里香','周杰伦','七里香',300,0,'2026-02-04 09:22:25');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (7,'gotest123','/uploads/music/周杰伦 - 七里香.mp3','七里香','周杰伦','七里香',300,0,'2026-02-04 09:22:26');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (8,'gotest123','/sdcard/Music/薛之谦 - 演员.mp3','演员','薛之谦','绅士',263,1,'2026-02-04 09:22:27');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (9,'gotest123','/uploads/music/周杰伦 - 七里香.mp3','七里香','周杰伦','七里香',300,0,'2026-02-04 09:23:51');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (10,'gotest123','/uploads/music/周杰伦 - 七里香.mp3','七里香','周杰伦','七里香',300,0,'2026-02-04 09:23:52');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (11,'gotest123','/uploads/music/周杰伦 - 七里香.mp3','七里香','周杰伦','七里香',300,0,'2026-02-04 09:23:53');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (12,'gotest123','/sdcard/Music/薛之谦 - 演员.mp3','演员','薛之谦','绅士',263,1,'2026-02-04 09:23:54');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (21,'gotest123','/uploads/music/周杰伦 - 七里香.mp3','七里香','周杰伦','七里香',300,0,'2026-02-04 12:27:46');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (22,'gotest123','/uploads/music/周杰伦 - 七里香.mp3','七里香','周杰伦','七里香',300,0,'2026-02-04 12:27:47');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (23,'gotest123','/uploads/music/周杰伦 - 七里香.mp3','七里香','周杰伦','七里香',300,0,'2026-02-04 12:27:48');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (24,'gotest123','/sdcard/Music/薛之谦 - 演员.mp3','演员','薛之谦','绅士',263,1,'2026-02-04 12:27:49');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (36,'gotest123','告白气球/告白气球.mp3','告白气球','周杰伦','周杰伦的床边故事',203,0,'2026-02-04 13:29:45');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (40,'gotest123','彩虹/彩虹.mp3','彩虹','周杰伦','我很忙',265,0,'2026-02-04 13:54:04');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (41,'gotest123','/sdcard/Music/薛之谦 - 演员.mp3','演员','薛之谦','绅士',263,1,'2026-02-04 13:54:05');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (42,'gotest123','枫/枫.mp3','枫','周杰伦','十一月的萧邦',278,0,'2026-02-04 13:54:06');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (93,'root','http://slcdut.xyz:8080/uploads/床边故事/床边故事.mp3','床边故事','周杰伦','',0,0,'2026-02-24 12:17:03');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (94,'root','http://slcdut.xyz:8080/uploads/床边故事/床边故事.mp3','床边故事','周杰伦','',0,0,'2026-02-24 12:20:51');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (115,'root','http://slcdut.xyz:8080/uploads/床边故事/床边故事.mp3','床边故事','周杰伦','',226,0,'2026-02-26 10:27:35');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (116,'root','E:/MP3/爱情废柴/爱情废柴.mp3','爱情废柴','周杰伦','',285,1,'2026-02-26 10:27:52');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (117,'root','http://slcdut.xyz:8080/uploads/给我一首歌的时间/给我一首歌的时间.mp3','给我一首歌的时间','周杰伦','',253,0,'2026-02-26 10:37:42');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (118,'root','http://slcdut.xyz:8080/uploads/花海/花海.mp3','花海','周杰伦','',264,0,'2026-02-26 10:38:45');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (119,'root','http://slcdut.xyz:8080/uploads/千里之外/千里之外.mp3','千里之外','周杰伦&费玉清','依然范特西',254.2,0,'2026-02-26 12:06:39');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (120,'root','http://slcdut.xyz:8080/uploads/花海/花海.mp3','花海','周杰伦','',264,0,'2026-02-26 12:09:33');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (121,'root','http://slcdut.xyz:8080/uploads/test/test.mp3','test-song','test-artist','test-album',123,0,'2026-02-26 12:13:00');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (122,'root','http://slcdut.xyz:8080/uploads/stream-check/stream-check.mp3','stream-check','qa','qa',66,0,'2026-02-26 13:24:52');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (123,'root','http://slcdut.xyz:8080/uploads/pending-test-1772113161351965157/pending-test-1772113161351965157.mp3','pending-test-1772113161351965157','qa','qa',11,0,'2026-02-26 13:39:21');
/*!40000 ALTER TABLE `user_play_history` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `users`
--

LOCK TABLES `users` WRITE;
/*!40000 ALTER TABLE `users` DISABLE KEYS */;
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('1008601','123456','shabi');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('123456','123456','123465');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('859565','shen2003','859565');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('859565175','123456','shen');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('999','123456','123456789');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('account_1756483326','testpass','testuser_1756483326');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('api_u_1772115150350225251','p@ss123','api_user_1772115150350225251');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('api_u_1772115337833212526','p@ss123','api_user_1772115337833212526');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('doc_u_1772115677003893249','DocPass123!','doc_user_1772115677003893249');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('doc_u_1772115738736798283','DocPass123!','doc_user_1772115738736798283');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('gotest123','gotest456','gotest');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('probe_reg_1772103205','123','probe_reg_1772103205');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('QQQ','123456','QQQ');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('root','123456','SL');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('root_','123456','sdfsfd');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('root__','123456','aaa');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('root00','123456','sll');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('root111','123456','SLSL');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('shen1','123456','SSSS');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('shenL','shen2003','shenLiang');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('SLL','123456','SL2');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('test_user_3593','secure_password_123','Test User');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('test_user_old','123456','测试用户旧版');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('test001','123456','测试用户');
INSERT INTO `users` (`account`, `password`, `username`) VALUES ('testaccount','testpass','testuser');
/*!40000 ALTER TABLE `users` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Current Database: `music_profile`
--

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `music_profile` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci */ /*!80016 DEFAULT ENCRYPTION='N' */;

USE `music_profile`;

--
-- Dumping data for table `schema_migrations`
--

LOCK TABLES `schema_migrations` WRITE;
/*!40000 ALTER TABLE `schema_migrations` DISABLE KEYS */;
INSERT INTO `schema_migrations` (`version`, `checksum`, `applied_at`) VALUES ('20260226_profile_schema_split.sql','0d9ab0b478c3e8a760bfb555cccbb4299f204ebc2cc5507e7379224f373a03b6','2026-02-26 13:57:39');
/*!40000 ALTER TABLE `schema_migrations` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `user_favorite_music`
--

LOCK TABLES `user_favorite_music` WRITE;
/*!40000 ALTER TABLE `user_favorite_music` DISABLE KEYS */;
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (1,'gotest123','/sdcard/Music/林俊杰 - 江南.mp3','江南','林俊杰',260,1,'2026-02-04 09:22:23');
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (2,'gotest123','千里之外/千里之外.mp3','千里之外','周杰伦/费玉清',269,0,'2026-02-04 13:54:04');
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (3,'gotest123','告白气球/告白气球.mp3','告白气球','周杰伦',203,0,'2026-02-04 13:54:04');
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (4,'root','白色风车/白色风车.mp3','白色风车.mp3','周杰伦',270,0,'2026-02-04 14:28:28');
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (5,'root','给我一首歌的时间/给我一首歌的时间.mp3','给我一首歌的时间.mp3','周杰伦',253,0,'2026-02-04 14:28:32');
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (6,'root','E:/MP3/爱情废柴/爱情废柴.mp3','爱情废柴.mp3','未知艺术家',285,1,'2026-02-26 06:16:23');
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (7,'root','轨迹/轨迹.mp3','轨迹.mp3','周杰伦',326,0,'2026-02-26 09:12:00');
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (8,'root','http://slcdut.xyz:8080/uploads/床边故事/床边故事.mp3','床边故事','周杰伦',226,0,'2026-02-26 10:28:01');
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (9,'root','花海/花海.mp3','花海.mp3','周杰伦',264,0,'2026-02-26 10:38:38');
INSERT INTO `user_favorite_music` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `duration_sec`, `is_local`, `created_at`) VALUES (10,'root','http://slcdut.xyz:8080/uploads/千里之外/千里之外.mp3','千里之外','周杰伦&费玉清',254.2,0,'2026-02-26 12:06:39');
/*!40000 ALTER TABLE `user_favorite_music` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `user_play_history`
--

LOCK TABLES `user_play_history` WRITE;
/*!40000 ALTER TABLE `user_play_history` DISABLE KEYS */;
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (1,'gotest123','/uploads/music/周杰伦 - 七里香.mp3','七里香','周杰伦','七里香',300,0,'2026-02-04 09:22:24');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (2,'gotest123','/uploads/music/周杰伦 - 七里香.mp3','七里香','周杰伦','七里香',300,0,'2026-02-04 09:22:25');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (3,'gotest123','/uploads/music/周杰伦 - 七里香.mp3','七里香','周杰伦','七里香',300,0,'2026-02-04 09:22:26');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (4,'gotest123','/sdcard/Music/薛之谦 - 演员.mp3','演员','薛之谦','绅士',263,1,'2026-02-04 09:22:27');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (5,'gotest123','/uploads/music/周杰伦 - 七里香.mp3','七里香','周杰伦','七里香',300,0,'2026-02-04 09:23:51');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (6,'gotest123','/uploads/music/周杰伦 - 七里香.mp3','七里香','周杰伦','七里香',300,0,'2026-02-04 09:23:52');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (7,'gotest123','/uploads/music/周杰伦 - 七里香.mp3','七里香','周杰伦','七里香',300,0,'2026-02-04 09:23:53');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (8,'gotest123','/sdcard/Music/薛之谦 - 演员.mp3','演员','薛之谦','绅士',263,1,'2026-02-04 09:23:54');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (9,'gotest123','/uploads/music/周杰伦 - 七里香.mp3','七里香','周杰伦','七里香',300,0,'2026-02-04 12:27:46');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (10,'gotest123','/uploads/music/周杰伦 - 七里香.mp3','七里香','周杰伦','七里香',300,0,'2026-02-04 12:27:47');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (11,'gotest123','/uploads/music/周杰伦 - 七里香.mp3','七里香','周杰伦','七里香',300,0,'2026-02-04 12:27:48');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (12,'gotest123','/sdcard/Music/薛之谦 - 演员.mp3','演员','薛之谦','绅士',263,1,'2026-02-04 12:27:49');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (13,'gotest123','告白气球/告白气球.mp3','告白气球','周杰伦','周杰伦的床边故事',203,0,'2026-02-04 13:29:45');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (14,'gotest123','彩虹/彩虹.mp3','彩虹','周杰伦','我很忙',265,0,'2026-02-04 13:54:04');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (15,'gotest123','/sdcard/Music/薛之谦 - 演员.mp3','演员','薛之谦','绅士',263,1,'2026-02-04 13:54:05');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (16,'gotest123','枫/枫.mp3','枫','周杰伦','十一月的萧邦',278,0,'2026-02-04 13:54:06');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (17,'root','http://slcdut.xyz:8080/uploads/床边故事/床边故事.mp3','床边故事','周杰伦','',0,0,'2026-02-24 12:17:03');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (18,'root','http://slcdut.xyz:8080/uploads/床边故事/床边故事.mp3','床边故事','周杰伦','',0,0,'2026-02-24 12:20:51');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (19,'root','http://slcdut.xyz:8080/uploads/床边故事/床边故事.mp3','床边故事','周杰伦','',226,0,'2026-02-26 10:27:35');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (20,'root','E:/MP3/爱情废柴/爱情废柴.mp3','爱情废柴','周杰伦','',285,1,'2026-02-26 10:27:52');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (21,'root','http://slcdut.xyz:8080/uploads/给我一首歌的时间/给我一首歌的时间.mp3','给我一首歌的时间','周杰伦','',253,0,'2026-02-26 10:37:42');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (22,'root','http://slcdut.xyz:8080/uploads/花海/花海.mp3','花海','周杰伦','',264,0,'2026-02-26 10:38:45');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (23,'root','http://slcdut.xyz:8080/uploads/千里之外/千里之外.mp3','千里之外','周杰伦&费玉清','依然范特西',254.2,0,'2026-02-26 12:06:39');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (24,'root','http://slcdut.xyz:8080/uploads/花海/花海.mp3','花海','周杰伦','',264,0,'2026-02-26 12:09:33');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (36,'root','http://slcdut.xyz:8080/uploads/千里之外/千里之外.mp3','千里之外','周杰伦&费玉清','',254,0,'2026-02-26 15:05:17');
INSERT INTO `user_play_history` (`id`, `user_account`, `music_path`, `music_title`, `artist`, `album`, `duration_sec`, `is_local`, `play_time`) VALUES (37,'root','http://slcdut.xyz:8080/uploads/花海/花海.mp3','花海','周杰伦','',264,0,'2026-02-26 15:05:22');
/*!40000 ALTER TABLE `user_play_history` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Current Database: `music_media`
--

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `music_media` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci */ /*!80016 DEFAULT ENCRYPTION='N' */;

USE `music_media`;

--
-- Dumping data for table `media_lyrics_map`
--

LOCK TABLES `media_lyrics_map` WRITE;
/*!40000 ALTER TABLE `media_lyrics_map` DISABLE KEYS */;
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (1,'hua_hai/hua_hai.mp3','hua_hai/hua_hai.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (2,'千里之外/千里之外.mp3','千里之外/千里之外.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (3,'彩虹/彩虹.mp3','彩虹/彩虹.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (4,'告白气球/告白气球.mp3','告白气球/告白气球.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (5,'周杰伦 - 七里香_EM/周杰伦 - 七里香_EM.flac','周杰伦 - 七里香_EM/周杰伦 - 七里香_EM.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (6,'枫/枫.mp3','枫/枫.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (7,'稻香/稻香.mp3','稻香/稻香.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (8,'发如雪/发如雪.mp3','发如雪/发如雪.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (9,'安静/安静.mp3','安静/安静.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (10,'花海/花海.mp3','花海/花海.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (11,'本草纲目/本草纲目.mp3','本草纲目/本草纲目.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (12,'白色风车/白色风车.mp3','白色风车/白色风车.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (13,'给我一首歌的时间/给我一首歌的时间.mp3','给我一首歌的时间/给我一首歌的时间.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (14,'东风破/东风破.mp3','东风破/东风破.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (15,'搁浅/搁浅.mp3','搁浅/搁浅.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (16,'断了的弦/断了的弦.mp3','断了的弦/断了的弦.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (17,'BEYOND - 海阔天空/BEYOND - 海阔天空.ogg','BEYOND - 海阔天空/BEYOND - 海阔天空.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (18,'Love_Song/Love_Song.mp3','Love_Song/Love_Song.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (19,'床边故事/床边故事.mp3','床边故事/床边故事.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (20,'暗号/暗号.mp3','暗号/暗号.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (21,'爱在西元前/爱在西元前.mp3','爱在西元前/爱在西元前.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (22,'yi_lu_xiang_bei/yi_lu_xiang_bei.mp3','yi_lu_xiang_bei/yi_lu_xiang_bei.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (23,'半兽人/半兽人.mp3','半兽人/半兽人.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (24,'爱情废柴/爱情废柴.mp3','爱情废柴/爱情废柴.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (25,'红尘客栈/红尘客栈.mp3','红尘客栈/红尘客栈.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (26,'qi_li_xiang/qi_li_xiang.ogg','qi_li_xiang/qi_li_xiang.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (27,'不爱我就拉倒/不爱我就拉倒.mp3','不爱我就拉倒/不爱我就拉倒.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (28,'轨迹/轨迹.mp3','轨迹/轨迹.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (29,'Gentle Bones&林俊杰-At Least I Had You/Gentle Bones&林俊杰-At Least I Had You.mp3','Gentle Bones&林俊杰-At Least I Had You/Gentle Bones&林俊杰-At Least I Had You.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (30,'Luis Fonsi&林俊杰-Despacito 缓缓 (Mandarin Version)/Luis Fonsi&林俊杰-Despacito 缓缓 (Mandarin Version).mp3','Luis Fonsi&林俊杰-Despacito 缓缓 (Mandarin Version)/Luis Fonsi&林俊杰-Despacito 缓缓 (Mandarin Version).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (31,'五月天&林俊杰-不为谁而作的歌/五月天&林俊杰-不为谁而作的歌.mp3','五月天&林俊杰-不为谁而作的歌/五月天&林俊杰-不为谁而作的歌.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (32,'五月天&林俊杰-知足 (2024「回到那一天」25周年巡回演唱会上海站) (现场)/五月天&林俊杰-知足 (2024「回到那一天」25周年巡回演唱会上海站) (现场).mp3','五月天&林俊杰-知足 (2024「回到那一天」25周年巡回演唱会上海站) (现场)/五月天&林俊杰-知足 (2024「回到那一天」25周年巡回演唱会上海站) (现场).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (33,'周华健&谢霆锋&容祖儿&林俊杰&张明敏-明天会更好 (2008中央电视台爱的奉献抗震救灾募捐晚会现场)/周华健&谢霆锋&容祖儿&林俊杰&张明敏-明天会更好 (2008中央电视台爱的奉献抗震救灾募捐晚会现场).mp3','周华健&谢霆锋&容祖儿&林俊杰&张明敏-明天会更好 (2008中央电视台爱的奉献抗震救灾募捐晚会现场)/周华健&谢霆锋&容祖儿&林俊杰&张明敏-明天会更好 (2008中央电视台爱的奉献抗震救灾募捐晚会现场).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (34,'周杰伦&林俊杰-Stay With You (50秒Live片段)/周杰伦&林俊杰-Stay With You (50秒Live片段).mp3','周杰伦&林俊杰-Stay With You (50秒Live片段)/周杰伦&林俊杰-Stay With You (50秒Live片段).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (35,'周杰伦&林俊杰-修炼爱情+最长的电影 (2017地表最强演唱会台北站)/周杰伦&林俊杰-修炼爱情+最长的电影 (2017地表最强演唱会台北站).mp3','周杰伦&林俊杰-修炼爱情+最长的电影 (2017地表最强演唱会台北站)/周杰伦&林俊杰-修炼爱情+最长的电影 (2017地表最强演唱会台北站).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (36,'周杰伦&林俊杰-晴天 (2017周杰伦地表最强演唱会台北站)/周杰伦&林俊杰-晴天 (2017周杰伦地表最强演唱会台北站).mp3','周杰伦&林俊杰-晴天 (2017周杰伦地表最强演唱会台北站)/周杰伦&林俊杰-晴天 (2017周杰伦地表最强演唱会台北站).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (37,'周杰伦&林俊杰-晴天+安静+修炼爱情+最长的电影 (2017周杰伦地表最强演唱会台北站)/周杰伦&林俊杰-晴天+安静+修炼爱情+最长的电影 (2017周杰伦地表最强演唱会台北站).mp3','周杰伦&林俊杰-晴天+安静+修炼爱情+最长的电影 (2017周杰伦地表最强演唱会台北站)/周杰伦&林俊杰-晴天+安静+修炼爱情+最长的电影 (2017周杰伦地表最强演唱会台北站).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (38,'周杰伦&林俊杰-算什么男人 (Live)/周杰伦&林俊杰-算什么男人 (Live).mp3','周杰伦&林俊杰-算什么男人 (Live)/周杰伦&林俊杰-算什么男人 (Live).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (39,'周深&林俊杰-大鱼 (Live)/周深&林俊杰-大鱼 (Live).mp3','周深&林俊杰-大鱼 (Live)/周深&林俊杰-大鱼 (Live).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (40,'周深&林俊杰-裹着心的光 (Live)/周深&林俊杰-裹着心的光 (Live).mp3','周深&林俊杰-裹着心的光 (Live)/周深&林俊杰-裹着心的光 (Live).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (41,'孙燕姿&林俊杰-我怀念的 (Live)/孙燕姿&林俊杰-我怀念的 (Live).mp3','孙燕姿&林俊杰-我怀念的 (Live)/孙燕姿&林俊杰-我怀念的 (Live).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (42,'张杰&林俊杰-最美的太阳 + 翅膀 (Live)/张杰&林俊杰-最美的太阳 + 翅膀 (Live).mp3','张杰&林俊杰-最美的太阳 + 翅膀 (Live)/张杰&林俊杰-最美的太阳 + 翅膀 (Live).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (43,'张真源-背对背拥抱 (片段)/张真源-背对背拥抱 (片段).mp3','张真源-背对背拥抱 (片段)/张真源-背对背拥抱 (片段).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (44,'徐佳莹&林俊杰-不为谁而作的歌 (Live)/徐佳莹&林俊杰-不为谁而作的歌 (Live).mp3','徐佳莹&林俊杰-不为谁而作的歌 (Live)/徐佳莹&林俊杰-不为谁而作的歌 (Live).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (45,'方大同&薛凯琪-四人遊/方大同&薛凯琪-四人遊.dsf','方大同&薛凯琪-四人遊/方大同&薛凯琪-四人遊.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (46,'方大同-Goodbye Melody Rose/方大同-Goodbye Melody Rose.dsf','方大同-Goodbye Melody Rose/方大同-Goodbye Melody Rose.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (47,'方大同-If You Leave Me Now/方大同-If You Leave Me Now.dsf','方大同-If You Leave Me Now/方大同-If You Leave Me Now.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (48,'方大同-Love Outrolude(Instrumental)/方大同-Love Outrolude(Instrumental).dsf','方大同-Love Outrolude(Instrumental)/方大同-Love Outrolude(Instrumental).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (49,'方大同-偷笑/方大同-偷笑.dsf','方大同-偷笑/方大同-偷笑.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (50,'方大同-唉!/方大同-唉!.dsf','方大同-唉!/方大同-唉!.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (51,'方大同-手拖手/方大同-手拖手.dsf','方大同-手拖手/方大同-手拖手.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (52,'方大同-拖男带女/方大同-拖男带女.dsf','方大同-拖男带女/方大同-拖男带女.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (53,'方大同-春风吹之吹吹风mix(吹吹风Mix)/方大同-春风吹之吹吹风mix(吹吹风Mix).dsf','方大同-春风吹之吹吹风mix(吹吹风Mix)/方大同-春风吹之吹吹风mix(吹吹风Mix).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (54,'方大同-歌手与模特儿/方大同-歌手与模特儿.dsf','方大同-歌手与模特儿/方大同-歌手与模特儿.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (55,'方大同-爱爱爱/方大同-爱爱爱.dsf','方大同-爱爱爱/方大同-爱爱爱.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (56,'方大同-苏丽珍/方大同-苏丽珍.dsf','方大同-苏丽珍/方大同-苏丽珍.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (57,'方大同-诗人的情人/方大同-诗人的情人.dsf','方大同-诗人的情人/方大同-诗人的情人.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (58,'林俊杰&张韶涵-保护色/林俊杰&张韶涵-保护色.flac','林俊杰&张韶涵-保护色/林俊杰&张韶涵-保护色.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (59,'林俊杰-Down(Demo)/林俊杰-Down(Demo).flac','林俊杰-Down(Demo)/林俊杰-Down(Demo).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (60,'林俊杰-I AM/林俊杰-I AM.flac','林俊杰-I AM/林俊杰-I AM.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (61,'林俊杰-Now That She\'s Gone/林俊杰-Now That She\'s Gone.flac','林俊杰-Now That She\'s Gone/林俊杰-Now That She\'s Gone.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (62,'林俊杰-一个又一个/林俊杰-一个又一个.mp3','林俊杰-一个又一个/林俊杰-一个又一个.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (63,'林俊杰-一千年以前/林俊杰-一千年以前.mp3','林俊杰-一千年以前/林俊杰-一千年以前.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (64,'林俊杰-一千年以后/林俊杰-一千年以后.mp3','林俊杰-一千年以后/林俊杰-一千年以后.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (65,'林俊杰-一千年以后 & 学不会 山东卫视2013新年演唱会 现场版 12、12、31/林俊杰-一千年以后 & 学不会 山东卫视2013新年演唱会 现场版 12、12、31.mp3','林俊杰-一千年以后 & 学不会 山东卫视2013新年演唱会 现场版 12、12、31/林俊杰-一千年以后 & 学不会 山东卫视2013新年演唱会 现场版 12、12、31.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (66,'林俊杰-一千年以后 (Live)/林俊杰-一千年以后 (Live).mp3','林俊杰-一千年以后 (Live)/林俊杰-一千年以后 (Live).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (67,'林俊杰-一千年后，记得我/林俊杰-一千年后，记得我.mp3','林俊杰-一千年后，记得我/林俊杰-一千年后，记得我.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (68,'林俊杰-一定会/林俊杰-一定会.mp3','林俊杰-一定会/林俊杰-一定会.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (69,'林俊杰-一时的选择/林俊杰-一时的选择.mp3','林俊杰-一时的选择/林俊杰-一时的选择.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (70,'林俊杰-一时的选择 (Live)/林俊杰-一时的选择 (Live).mp3','林俊杰-一时的选择 (Live)/林俊杰-一时的选择 (Live).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (71,'林俊杰-一生的爱/林俊杰-一生的爱.flac','林俊杰-一生的爱/林俊杰-一生的爱.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (72,'林俊杰-一眼万年/林俊杰-一眼万年.flac','林俊杰-一眼万年/林俊杰-一眼万年.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (73,'林俊杰-一路向北/林俊杰-一路向北.mp3','林俊杰-一路向北/林俊杰-一路向北.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (74,'林俊杰-不死之身/林俊杰-不死之身.flac','林俊杰-不死之身/林俊杰-不死之身.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (75,'林俊杰-为什么相爱的人不能在一起/林俊杰-为什么相爱的人不能在一起.mp3','林俊杰-为什么相爱的人不能在一起/林俊杰-为什么相爱的人不能在一起.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (76,'林俊杰-主角/林俊杰-主角.mp3','林俊杰-主角/林俊杰-主角.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (77,'林俊杰-以后要做的事/林俊杰-以后要做的事.mp3','林俊杰-以后要做的事/林俊杰-以后要做的事.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (78,'林俊杰-伟大的渺小/林俊杰-伟大的渺小.mp3','林俊杰-伟大的渺小/林俊杰-伟大的渺小.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (79,'林俊杰-伟大的渺小 (Jazz Version)/林俊杰-伟大的渺小 (Jazz Version).mp3','林俊杰-伟大的渺小 (Jazz Version)/林俊杰-伟大的渺小 (Jazz Version).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (80,'林俊杰-你要的不是我/林俊杰-你要的不是我.flac','林俊杰-你要的不是我/林俊杰-你要的不是我.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (81,'林俊杰-修炼爱情/林俊杰-修炼爱情.mp3','林俊杰-修炼爱情/林俊杰-修炼爱情.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (82,'林俊杰-修炼爱情 (2013 Hito流行音乐奖颁奖典礼现场)/林俊杰-修炼爱情 (2013 Hito流行音乐奖颁奖典礼现场).mp3','林俊杰-修炼爱情 (2013 Hito流行音乐奖颁奖典礼现场)/林俊杰-修炼爱情 (2013 Hito流行音乐奖颁奖典礼现场).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (83,'林俊杰-修炼爱情 (2015台北跨年演唱会)/林俊杰-修炼爱情 (2015台北跨年演唱会).mp3','林俊杰-修炼爱情 (2015台北跨年演唱会)/林俊杰-修炼爱情 (2015台北跨年演唱会).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (84,'林俊杰-修炼爱情 (Instrumental Version)/林俊杰-修炼爱情 (Instrumental Version).mp3','林俊杰-修炼爱情 (Instrumental Version)/林俊杰-修炼爱情 (Instrumental Version).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (85,'林俊杰-修炼爱情 (Jazz Version)/林俊杰-修炼爱情 (Jazz Version).mp3','林俊杰-修炼爱情 (Jazz Version)/林俊杰-修炼爱情 (Jazz Version).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (86,'林俊杰-修炼爱情 (Live)/林俊杰-修炼爱情 (Live).mp3','林俊杰-修炼爱情 (Live)/林俊杰-修炼爱情 (Live).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (87,'林俊杰-修炼爱情 + 黑夜问白天 + 交换余生 + 那些你很冒险的梦 (2023 bilibili毕业歌会现场)/林俊杰-修炼爱情 + 黑夜问白天 + 交换余生 + 那些你很冒险的梦 (2023 bilibili毕业歌会现场).mp3','林俊杰-修炼爱情 + 黑夜问白天 + 交换余生 + 那些你很冒险的梦 (2023 bilibili毕业歌会现场)/林俊杰-修炼爱情 + 黑夜问白天 + 交换余生 + 那些你很冒险的梦 (2023 bilibili毕业歌会现场).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (88,'林俊杰-像我这样的人 (28秒2018梦想的声音第三季第9期现场片段)/林俊杰-像我这样的人 (28秒2018梦想的声音第三季第9期现场片段).mp3','林俊杰-像我这样的人 (28秒2018梦想的声音第三季第9期现场片段)/林俊杰-像我这样的人 (28秒2018梦想的声音第三季第9期现场片段).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (89,'林俊杰-压力/林俊杰-压力.mp3','林俊杰-压力/林俊杰-压力.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (90,'林俊杰-原来/林俊杰-原来.flac','林俊杰-原来/林俊杰-原来.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (91,'林俊杰-只对你有感觉/林俊杰-只对你有感觉.flac','林俊杰-只对你有感觉/林俊杰-只对你有感觉.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (92,'林俊杰-只对你说 (Live)/林俊杰-只对你说 (Live).mp3','林俊杰-只对你说 (Live)/林俊杰-只对你说 (Live).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (93,'林俊杰-只要有你的地方/林俊杰-只要有你的地方.mp3','林俊杰-只要有你的地方/林俊杰-只要有你的地方.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (94,'林俊杰-只要有你的地方 (晚安版)/林俊杰-只要有你的地方 (晚安版).mp3','林俊杰-只要有你的地方 (晚安版)/林俊杰-只要有你的地方 (晚安版).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (95,'林俊杰-因你而在/林俊杰-因你而在.mp3','林俊杰-因你而在/林俊杰-因你而在.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (96,'林俊杰-她说/林俊杰-她说.flac','林俊杰-她说/林俊杰-她说.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (97,'林俊杰-子弹列车/林俊杰-子弹列车.mp3','林俊杰-子弹列车/林俊杰-子弹列车.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (98,'林俊杰-学不会/林俊杰-学不会.mp3','林俊杰-学不会/林俊杰-学不会.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (99,'林俊杰-学不会(伴奏版)/林俊杰-学不会(伴奏版).mp3','林俊杰-学不会(伴奏版)/林俊杰-学不会(伴奏版).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (100,'林俊杰-完美新世界/林俊杰-完美新世界.flac','林俊杰-完美新世界/林俊杰-完美新世界.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (101,'林俊杰-小瓶子/林俊杰-小瓶子.mp3','林俊杰-小瓶子/林俊杰-小瓶子.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (102,'林俊杰-小酒窝/林俊杰-小酒窝.mp3','林俊杰-小酒窝/林俊杰-小酒窝.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (103,'林俊杰-小酒窝 (2023马栏山芒果节不设限毕业礼现场) (Live)/林俊杰-小酒窝 (2023马栏山芒果节不设限毕业礼现场) (Live).mp3','林俊杰-小酒窝 (2023马栏山芒果节不设限毕业礼现场) (Live)/林俊杰-小酒窝 (2023马栏山芒果节不设限毕业礼现场) (Live).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (104,'林俊杰-小酒窝 (Live)/林俊杰-小酒窝 (Live).mp3','林俊杰-小酒窝 (Live)/林俊杰-小酒窝 (Live).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (105,'林俊杰-小酒窝 (胡彦斌 男人 ktv 江南) (其他)/林俊杰-小酒窝 (胡彦斌 男人 ktv 江南) (其他).mp3','林俊杰-小酒窝 (胡彦斌 男人 ktv 江南) (其他)/林俊杰-小酒窝 (胡彦斌 男人 ktv 江南) (其他).lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (106,'林俊杰-幸存者/林俊杰-幸存者.mp3','林俊杰-幸存者/林俊杰-幸存者.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (107,'林俊杰-当你/林俊杰-当你.flac','林俊杰-当你/林俊杰-当你.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (108,'林俊杰-心墙/林俊杰-心墙.flac','林俊杰-心墙/林俊杰-心墙.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (109,'林俊杰-我很想爱他/林俊杰-我很想爱他.flac','林俊杰-我很想爱他/林俊杰-我很想爱他.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (110,'林俊杰-握不住的他/林俊杰-握不住的他.flac','林俊杰-握不住的他/林俊杰-握不住的他.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (111,'林俊杰-曹操/林俊杰-曹操.flac','林俊杰-曹操/林俊杰-曹操.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (112,'林俊杰-波间带/林俊杰-波间带.flac','林俊杰-波间带/林俊杰-波间带.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (113,'林俊杰-流行主教/林俊杰-流行主教.flac','林俊杰-流行主教/林俊杰-流行主教.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (114,'林俊杰-熟能生巧/林俊杰-熟能生巧.flac','林俊杰-熟能生巧/林俊杰-熟能生巧.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (115,'林俊杰-爱情Yogurt/林俊杰-爱情Yogurt.flac','林俊杰-爱情Yogurt/林俊杰-爱情Yogurt.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (116,'林俊杰-爱笑的眼睛/林俊杰-爱笑的眼睛.flac','林俊杰-爱笑的眼睛/林俊杰-爱笑的眼睛.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (117,'林俊杰-真材实料的我/林俊杰-真材实料的我.flac','林俊杰-真材实料的我/林俊杰-真材实料的我.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (118,'林俊杰-记得/林俊杰-记得.flac','林俊杰-记得/林俊杰-记得.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (119,'林俊杰-进化论/林俊杰-进化论.flac','林俊杰-进化论/林俊杰-进化论.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
INSERT INTO `media_lyrics_map` (`id`, `music_path`, `lrc_path`, `source`, `created_at`, `updated_at`) VALUES (120,'林俊杰-사랑해요只对你说/林俊杰-사랑해요只对你说.flac','林俊杰-사랑해요只对你说/林俊杰-사랑해요只对你说.lrc','catalog','2026-02-26 13:57:39','2026-02-26 14:18:57');
/*!40000 ALTER TABLE `media_lyrics_map` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Dumping data for table `schema_migrations`
--

LOCK TABLES `schema_migrations` WRITE;
/*!40000 ALTER TABLE `schema_migrations` DISABLE KEYS */;
INSERT INTO `schema_migrations` (`version`, `checksum`, `applied_at`) VALUES ('20260226_media_schema_split.sql','c4ac7c502d8032f93f44fc4fbdd63ce9a8df6d99e69b1b69d0379fdb0ed89bb3','2026-02-26 13:57:39');
/*!40000 ALTER TABLE `schema_migrations` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-02-27 18:07:06
