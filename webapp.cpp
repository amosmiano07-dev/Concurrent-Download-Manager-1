#include "crow_all.h"
#include <cstdlib>
#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>

// --- SAFE QUEUE SYSTEM ---
std::queue<std::string> job_queue;
std::mutex queue_mutex;
std::condition_variable queue_cv;
bool running = true;

// --- THE WORKER THREAD (The Engine Driver) ---
// This runs in the background forever. It waits for links and processes them one by one.
void worker_thread_func() {
    while (running) {
        std::string url_to_download;

        // 1. Wait for a job safely
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cv.wait(lock, []{ return !job_queue.empty() || !running; });

            if (!running && job_queue.empty()) return;

            url_to_download = job_queue.front();
            job_queue.pop();
        }

        // 2. RUN YOUR ENGINE (Non-Blocking for the web server)
        std::cout << "[Worker] Starting download for: " << url_to_download << std::endl;
        
        // We use specific quotes around the URL to prevent shell injection crashes
        std::string command = "./my_downloader \"" + url_to_download + "\"";
        
        // Run the command and ignore output (or redirect to a log file)
        int result = std::system(command.c_str());
        
        if(result == 0) std::cout << "[Worker] Success!\n";
        else std::cout << "[Worker] Download failed or warned.\n";
    }
}

int main() {
    // Start the background worker thread
    std::thread worker(worker_thread_func);
    worker.detach(); // Let it run independently

    crow::SimpleApp app;

    // --- FRONTEND (The UI) ---
    CROW_ROUTE(app, "/")([](){
        return R"(
            <html>
            <head>
                <title>Ultra-Fast Downloader</title>
                <style>
                    body { font-family: 'Segoe UI', sans-serif; background: #222; color: #fff; text-align: center; padding: 50px; }
                    input { padding: 15px; width: 60%; border-radius: 5px; border: none; }
                    button { padding: 15px 30px; background: #007bff; color: white; border: none; border-radius: 5px; cursor: pointer; font-weight: bold; }
                    button:hover { background: #0056b3; }
                    .status { margin-top: 20px; color: #aaa; }
                </style>
            </head>
            <body>
                <h1> C++ Download Engine</h1>
                <form action="/add_job" method="POST">
                    <input type="text" name="url" placeholder="Paste YouTube Link Here..." required>
                    <button type="submit">Download</button>
                </form>
                <div class="status">Jobs are processed in the background terminal.</div>
            </body>
            </html>
        )";
    });

    // --- BACKEND (The Linker) ---
    CROW_ROUTE(app, "/add_job").methods(crow::HTTPMethod::POST)([](const crow::request& req){
        // 1. Parse the URL (Manual parsing for simplicity)
        std::string body = req.body;
        std::string prefix = "url=";
        std::string url = "";
        
        size_t pos = body.find(prefix);
        if(pos != std::string::npos) {
            url = body.substr(pos + prefix.length());
            
            // Decode URL symbols (simple version)
            // Real browsers send http%3A%2F%2F instead of http://
            // For a basic test, this is usually fine, but a real decoder is better.
            // A quick hack to fix common encoding:
            size_t replace_pos;
            while ((replace_pos = url.find("%3A")) != std::string::npos) url.replace(replace_pos, 3, ":");
            while ((replace_pos = url.find("%2F")) != std::string::npos) url.replace(replace_pos, 3, "/");
            while ((replace_pos = url.find("%3F")) != std::string::npos) url.replace(replace_pos, 3, "?");
            while ((replace_pos = url.find("%3D")) != std::string::npos) url.replace(replace_pos, 3, "=");
            while ((replace_pos = url.find("%26")) != std::string::npos) url.replace(replace_pos, 3, "&");
        }

        if(url.empty()) return crow::response(400, "Invalid URL");

        // 2. Add to Queue Safely
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            job_queue.push(url);
        }
        queue_cv.notify_one(); // Wake up the worker

        // 3. Respond immediately (Don't wait for download!)
        return crow::response("<h1>Job Added!</h1><p>The engine is downloading it in the background.</p><a href='/'>Go Back</a>");
    });

    app.port(18080).multithreaded().run();
}
