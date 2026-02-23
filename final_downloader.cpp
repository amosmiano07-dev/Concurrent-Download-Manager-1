#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <fstream>
#include <curl/curl.h>
#include <cmath>
#include <iomanip>
#include <cstdio>
#include <memory>
#include <array>

// --- Global Variables ---
std::mutex progress_mutex;
long long total_file_size = 0;
// We use a vector to track exactly how much EACH thread has downloaded
std::vector<long long> thread_progress; 

// --- 1. Helper to Run yt-dlp ---
std::string get_direct_link(std::string url) {
    std::array<char, 128> buffer;
    std::string result;
    // The "2>/dev/null" hides the yellow warnings
    std::string command = "yt-dlp -f 18 -g \"" + url + "\" 2>/dev/null"; 
    
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) result += buffer.data();
    
    // Remove newline at the end
    if (!result.empty() && result.back() == '\n') result.pop_back();
    return result;
}

// --- 2. The Write Function ---
struct ThreadData {
    int id;
    std::ofstream* stream;
};

size_t write_data(void* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t written = size * nmemb;
    ThreadData* data = (ThreadData*)userdata;
    
    // Write to disk
    data->stream->write((char*)ptr, written);
    
    // Update the SPECIFIC progress slot for this thread
    // No lock needed here because each thread only touches its own index
    thread_progress[data->id] += written;
    
    return written;
}

// --- 3. The Dashboard (Visuals) ---
void display_dashboard(int num_threads) {
    long long total_downloaded = 0;
    
    // Clear screen space for the bars
    for(int i=0; i<num_threads+1; i++) std::cout << "\n"; 

    while(total_downloaded < total_file_size) {
        // Move cursor UP to overwrite previous frame
        std::cout << "\033[" << num_threads + 1 << "A";

        total_downloaded = 0;
        
        for(int i = 0; i < num_threads; i++) {
            long long current = thread_progress[i];
            total_downloaded += current;
            
            // Calculate thread percentage
            long expected_share = total_file_size / num_threads;
            double percent = (double)current / expected_share * 100.0;
            if (percent > 100.0) percent = 100.0;

            // Draw Bar
            std::cout << "Thread " << i+1 << ": [";
            int barWidth = 40;
            int pos = barWidth * percent / 100;
            for (int j = 0; j < barWidth; ++j) {
                if (j < pos) std::cout << "#";
                else std::cout << " ";
            }
            std::cout << "] " << std::fixed << std::setprecision(1) << percent << "%   \n";
        }
        
        // Total Progress
        double total_percent = (double)total_downloaded / total_file_size * 100.0;
        std::cout << "TOTAL   : " << std::fixed << std::setprecision(1) << total_percent << "% " 
                  << "(" << total_downloaded/(1024*1024) << "MB / " << total_file_size/(1024*1024) << "MB)   \n";

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Refresh rate
        if (total_downloaded >= total_file_size) break;
    }
    std::cout << "--------------------------------------------------\n";
}

// --- 4. Worker Thread ---
void download_chunk(int id, std::string url, long start, long end) {
    CURL* curl = curl_easy_init();
    std::string filename = "part_" + std::to_string(id);
    
    if(curl) {
        std::ofstream outfile(filename, std::ios::binary);
        ThreadData data = {id, &outfile}; 
        
        std::string range = std::to_string(start) + "-" + std::to_string(end);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data); 
        curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        curl_easy_perform(curl);
        outfile.close();
        curl_easy_cleanup(curl);
    }
}

// --- 5. Helper & Merge Functions ---
double get_size(std::string url) {
    CURL* curl = curl_easy_init();
    double size = 0.0;
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &size);
        curl_easy_cleanup(curl);
    }
    return size;
}

void merge_files(int num_threads, std::string final_name) {
    std::cout << "Merging files..." << std::endl;
    std::ofstream outfile(final_name, std::ios::binary);
    for(int i = 0; i < num_threads; i++) {
        std::string part_name = "part_" + std::to_string(i);
        std::ifstream infile(part_name, std::ios::binary);
        outfile << infile.rdbuf();
        infile.close();
        remove(part_name.c_str());
    }
    outfile.close();
    std::cout << "Success! Saved as: " << final_name << std::endl;
}

int main(int argc, char* argv[]) {
    std::string youtube_url;
    if (argc > 1) youtube_url = argv[1];
    else {
        std::cout << "Enter YouTube URL: ";
        std::cin >> youtube_url;
    }

    std::cout << "Extracting URL..." << std::endl;
    std::string direct_url = get_direct_link(youtube_url);
    
    // Check if the link is empty (extraction failed)
    if (direct_url.empty()) {
        std::cout << "Failed to extract link. Please check the URL." << std::endl;
        return 1;
    }

    total_file_size = (long)get_size(direct_url);
    if (total_file_size <= 0) {
        std::cout << "Error: Could not get file size." << std::endl;
        return 1;
    }
    
    int num_threads = 4;
    // Initialize the progress tracker with 0s
    thread_progress.resize(num_threads, 0);

    std::cout << "Starting " << num_threads << " threads..." << std::endl;

    long chunk_size = total_file_size / num_threads;
    std::vector<std::thread> workers;

    for(int i = 0; i < num_threads; i++) {
        long start = i * chunk_size;
        long end = (i == num_threads - 1) ? total_file_size - 1 : (start + chunk_size - 1);
        workers.push_back(std::thread(download_chunk, i, direct_url, start, end));
    }

    // Run the UI
    display_dashboard(num_threads);

    for(auto& t : workers) t.join();
    merge_files(num_threads, "video.mp4");

    return 0;
}
