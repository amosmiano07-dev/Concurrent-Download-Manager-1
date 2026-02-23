# C++ Concurrent Download Manager

A high-performance, multi-threaded download manager and web server packaged into a single C++ file. 

Unlike traditional sequential downloaders, this engine optimizes bandwidth by splitting files into segments and downloading them simultaneously. It features a lightweight, responsive Web Interface for easy user interaction, bypassing the need for complex terminal commands.

## ✨ Key Features
* **Single-File Architecture:** The entire download engine, multithreading logic, and web server are efficiently contained within `webapp.cpp`.
* **Parallel Transfer Engine:** Maximizes download speeds by establishing multiple concurrent HTTP connections using `std::thread`.
* **HTTP Segmentation:** Utilizes HTTP `Range` requests to virtually "cut" files into manageable chunks before downloading.
* **Thread-Safe:** Implements `std::mutex` locking to prevent race conditions.
* **Modern Web Interface:** A sleek, browser-based frontend powered by the Crow C++ microframework.

## 🛠️ Prerequisites
To compile and run this project, you need a C++ compiler (`g++`) and the following libraries:
* **libcurl:** For handling HTTP requests and data transfers.
* **Crow:** A header-only C++ web framework.

**Ubuntu / Debian Installation:**
```bash
# Install the compiler and libcurl
sudo apt update
sudo apt install g++ libcurl4-openssl-dev

# Download the Crow header file to your project directory
wget [https://github.com/CrowCpp/Crow/releases/download/v1.0%2B5/crow_all.h](https://github.com/CrowCpp/Crow/releases/download/v1.0%2B5/crow_all.h) -O crow.h

🚀 **Quick Start (Build & Run)**
1. Clone the repository:

Bash
git clone [https://github.com/yourusername/concurrent-download-manager.git](https://github.com/yourusername/concurrent-download-manager.git)
cd concurrent-download-manager
2. Compile the application:
Because this is a single-file application, compilation is straightforward. Ensure you link both the pthread and curl libraries.

Bash
g++ webapp.cpp -o webapp -lcurl -lpthread
3. Start the server:

Bash
./webapp
4. Access the Interface:
Open your preferred web browser and navigate to:
http://localhost:18080

🛑 Common Troubleshooting
fatal error: crow.h: No such file or directory: Ensure you have downloaded crow_all.h and renamed it to crow.h in the same directory as webapp.cpp.

bind: Address already in use: The server was not shut down properly and is still holding the port. Find the process using lsof -i :18080 and terminate it using kill -9 <PID>.

📄 License
This project is open-source and available under the MIT License.
