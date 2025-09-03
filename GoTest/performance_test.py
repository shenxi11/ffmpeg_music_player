#!/usr/bin/env python3
"""
性能测试脚本 - 对比Python版本和Go版本音乐服务器
Python版本: http://localhost:5000 (Flask/Quart)
Go版本: http://localhost:8080 (Go + Redis)
"""

import asyncio
import aiohttp
import time
import json
import statistics
import sys
from typing import List, Dict, Any
import concurrent.futures
from dataclasses import dataclass

@dataclass
class TestResult:
    name: str
    python_times: List[float]
    go_times: List[float]
    python_success: int
    go_success: int
    python_errors: List[str]
    go_errors: List[str]

class PerformanceTest:
    def __init__(self):
        self.python_base_url = "http://localhost:5000"
        self.go_base_url = "http://122.51.73.110:8080"
        self.results: List[TestResult] = []
        
    async def test_endpoint(self, session: aiohttp.ClientSession, url: str, method: str = 'GET', data: Dict = None) -> tuple:
        """测试单个端点"""
        start_time = time.perf_counter()
        try:
            if method.upper() == 'POST':
                async with session.post(url, json=data) as response:
                    await response.text()
                    success = response.status < 400
            else:
                async with session.get(url) as response:
                    await response.text()
                    success = response.status < 400
            
            end_time = time.perf_counter()
            return end_time - start_time, success, None
        except Exception as e:
            end_time = time.perf_counter()
            return end_time - start_time, False, str(e)
    
    async def run_concurrent_test(self, endpoint: str, method: str = 'GET', data: Dict = None, 
                                concurrent_users: int = 10, requests_per_user: int = 5) -> TestResult:
        """并发测试特定端点"""
        print(f"\n🔄 测试端点: {endpoint} (方法: {method}, 并发: {concurrent_users}, 每用户请求: {requests_per_user})")
        
        python_url = f"{self.python_base_url}{endpoint}"
        go_url = f"{self.go_base_url}{endpoint}"
        
        python_times = []
        go_times = []
        python_errors = []
        go_errors = []
        python_success = 0
        go_success = 0
        
        async with aiohttp.ClientSession(timeout=aiohttp.ClientTimeout(total=30)) as session:
            # 测试Python版本
            print("  📊 测试Python版本...")
            tasks = []
            for _ in range(concurrent_users):
                for _ in range(requests_per_user):
                    task = self.test_endpoint(session, python_url, method, data)
                    tasks.append(task)
            
            python_results = await asyncio.gather(*tasks, return_exceptions=True)
            
            for result in python_results:
                if isinstance(result, Exception):
                    python_errors.append(str(result))
                else:
                    duration, success, error = result
                    python_times.append(duration)
                    if success:
                        python_success += 1
                    if error:
                        python_errors.append(error)
            
            # 测试Go版本
            print("  📊 测试Go版本...")
            tasks = []
            for _ in range(concurrent_users):
                for _ in range(requests_per_user):
                    task = self.test_endpoint(session, go_url, method, data)
                    tasks.append(task)
            
            go_results = await asyncio.gather(*tasks, return_exceptions=True)
            
            for result in go_results:
                if isinstance(result, Exception):
                    go_errors.append(str(result))
                else:
                    duration, success, error = result
                    go_times.append(duration)
                    if success:
                        go_success += 1
                    if error:
                        go_errors.append(error)
        
        return TestResult(
            name=endpoint,
            python_times=python_times,
            go_times=go_times,
            python_success=python_success,
            go_success=go_success,
            python_errors=python_errors,
            go_errors=go_errors
        )
    
    def calculate_stats(self, times: List[float]) -> Dict[str, float]:
        """计算统计数据"""
        if not times:
            return {"mean": 0, "median": 0, "min": 0, "max": 0, "std": 0}
        
        return {
            "mean": statistics.mean(times) * 1000,  # 转换为毫秒
            "median": statistics.median(times) * 1000,
            "min": min(times) * 1000,
            "max": max(times) * 1000,
            "std": statistics.stdev(times) * 1000 if len(times) > 1 else 0
        }
    
    def print_results(self):
        """打印测试结果"""
        print("\n" + "="*80)
        print("🎯 性能测试结果总结")
        print("="*80)
        
        for result in self.results:
            print(f"\n📋 端点: {result.name}")
            print("-" * 60)
            
            # Python统计
            python_stats = self.calculate_stats(result.python_times)
            print(f"🐍 Python版本 (Flask/Quart):")
            print(f"   成功请求: {result.python_success}/{len(result.python_times)}")
            print(f"   平均响应时间: {python_stats['mean']:.2f}ms")
            print(f"   中位数: {python_stats['median']:.2f}ms")
            print(f"   最小/最大: {python_stats['min']:.2f}ms / {python_stats['max']:.2f}ms")
            print(f"   标准差: {python_stats['std']:.2f}ms")
            if result.python_errors:
                print(f"   错误数: {len(result.python_errors)}")
            
            # Go统计
            go_stats = self.calculate_stats(result.go_times)
            print(f"🚀 Go版本 (Go + Redis):")
            print(f"   成功请求: {result.go_success}/{len(result.go_times)}")
            print(f"   平均响应时间: {go_stats['mean']:.2f}ms")
            print(f"   中位数: {go_stats['median']:.2f}ms")
            print(f"   最小/最大: {go_stats['min']:.2f}ms / {go_stats['max']:.2f}ms")
            print(f"   标准差: {go_stats['std']:.2f}ms")
            if result.go_errors:
                print(f"   错误数: {len(result.go_errors)}")
            
            # 性能对比
            if python_stats['mean'] > 0 and go_stats['mean'] > 0:
                improvement = ((python_stats['mean'] - go_stats['mean']) / python_stats['mean']) * 100
                print(f"📈 性能提升: {improvement:+.1f}% {'(Go更快)' if improvement > 0 else '(Python更快)'}")
                throughput_python = 1000 / python_stats['mean'] if python_stats['mean'] > 0 else 0
                throughput_go = 1000 / go_stats['mean'] if go_stats['mean'] > 0 else 0
                print(f"⚡ 吞吐量: Python {throughput_python:.1f} req/s, Go {throughput_go:.1f} req/s")
        
        # 总体统计
        print("\n" + "="*80)
        print("📊 总体性能统计")
        print("="*80)
        
        total_python_times = []
        total_go_times = []
        total_python_success = 0
        total_go_success = 0
        
        for result in self.results:
            total_python_times.extend(result.python_times)
            total_go_times.extend(result.go_times)
            total_python_success += result.python_success
            total_go_success += result.go_success
        
        python_overall = self.calculate_stats(total_python_times)
        go_overall = self.calculate_stats(total_go_times)
        
        print(f"🐍 Python总体: {python_overall['mean']:.2f}ms平均, 成功率 {total_python_success}/{len(total_python_times)}")
        print(f"🚀 Go总体: {go_overall['mean']:.2f}ms平均, 成功率 {total_go_success}/{len(total_go_times)}")
        
        if python_overall['mean'] > 0 and go_overall['mean'] > 0:
            overall_improvement = ((python_overall['mean'] - go_overall['mean']) / python_overall['mean']) * 100
            print(f"🎯 总体性能提升: {overall_improvement:+.1f}%")
            
        print("\n💡 测试结论:")
        if go_overall['mean'] < python_overall['mean']:
            print("✅ Go版本在响应时间上表现更优")
        else:
            print("⚠️  Python版本在响应时间上表现更优")
            
        if total_go_success > total_python_success:
            print("✅ Go版本在成功率上表现更优")
        elif total_go_success == total_python_success:
            print("🔄 两个版本成功率相当")
        else:
            print("⚠️  Python版本在成功率上表现更优")

    async def run_all_tests(self):
        """运行所有测试"""
        print("🚀 开始音乐服务器性能测试...")
        print(f"Python版本: {self.python_base_url}")
        print(f"Go版本: {self.go_base_url}")
        
        # 测试配置
        test_configs = [
            # 基础端点测试
            {"endpoint": "/", "method": "GET", "concurrent": 20, "requests": 10},
            
            # 文件列表测试 (重要性能点)
            {"endpoint": "/files", "method": "GET", "concurrent": 15, "requests": 8},
            
            # 用户登录测试
            {"endpoint": "/users/login", "method": "POST", 
             "data": {"account": "root", "password": "123456"}, 
             "concurrent": 10, "requests": 5},
            
            # 音频流测试
            {"endpoint": "/stream", "method": "POST", 
             "data": {"filename": "东风破.mp3"}, 
             "concurrent": 8, "requests": 3},
            
            # 文件搜索测试
            {"endpoint": "/file", "method": "POST", 
             "data": {"filename": "七里"}, 
             "concurrent": 12, "requests": 6},
             
            # 用户注册测试
            {"endpoint": "/users/register", "method": "POST", 
             "data": {"username": f"testuser_{int(time.time())}", 
                     "account": f"account_{int(time.time())}", 
                     "password": "testpass"}, 
             "concurrent": 5, "requests": 2},
        ]
        
        # 运行测试
        for config in test_configs:
            result = await self.run_concurrent_test(
                endpoint=config["endpoint"],
                method=config["method"],
                data=config.get("data"),
                concurrent_users=config["concurrent"],
                requests_per_user=config["requests"]
            )
            self.results.append(result)
            
            # 添加延迟避免服务器过载
            await asyncio.sleep(1)
        
        # 打印结果
        self.print_results()


async def check_go_service():
    """只检查Go服务是否运行"""
    async with aiohttp.ClientSession() as session:
        try:
            async with session.get("http://122.51.73.110:8080/") as resp:
                content = await resp.text()
                print(f"Go服务(8080端口)响应: 状态码={resp.status}, 内容={content[:60]}")
                return True
        except Exception as e:
            print(f"❌ Go服务(8080端口)无法连接: {e}")
            return False

async def main():
    print("🔍 检查Go服务状态...")
    if not await check_go_service():
        print("\n❌ 请确保Go服务在运行: ./main (端口8080)")
        sys.exit(1)
    print("\n⏱️  开始性能测试 (这可能需要几分钟)...")
    tester = PerformanceTest()
    await tester.run_all_tests()
    print("\n🎉 测试完成!")

if __name__ == "__main__":
    asyncio.run(main())
