# linux-file-transfer-server
基于Linux C语言实现C/S架构文件存储与传输管理系统，TCP Socket网络通信。

## 技术栈
- C语言
- Linux
- TCP Socket
- 多线程 pthread
- 标准IO
- SQLite3

## 项目简介
服务端采用多线程模型，支持多客户端并发接入；实现自定义应用层通信协议，支持用户注册登录、文件上传、下载、断点续传，使用SQLite管理账号与传输日志，增加数据校验保障传输可靠。

## 核心功能
1. 自定义应用层请求-响应协议，支持登录、注册、文件上传、下载、断点查询
2. 服务端多线程并发，每个客户端分配独立线程，支持多用户同时传输
3. lseek 文件偏移实现断点续传，大文件中断后可继续传输
4. 数据帧校验，保证网络传输数据完整性
5. SQLite3 存储用户账号、传输记录，支持登录校验、日志追溯

## 编译环境
- Linux（Ubuntu）
- gcc
- pthread
- SQLite3

## 编译命令
```bash
gcc server.c -o server -lpthread -lsqlite3
gcc client.c -o client -lpthread -lsqlite3
