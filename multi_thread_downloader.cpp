#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <fstream>
#include <curl/curl.h>
#include <cmath>
#include <iomanip>

// --- Global Variables to Manage Threads ---
std::mutex progress_mutex;
long long total_downloaded_bytes = 0;
long long total_file_size = 0;

// --- 1. The Write Function (Saves data to disk) ---
size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream) {
    size_t written = size * nmemb;
    std::ofstream* out = (std::ofstream*)stream;
    out->write((char*)ptr, written);
    
    // Lock the thread to safely update the progress bar
    {
        std::lock_guard<std::mutex> lock(progress_mutex);
        total_downloaded_bytes += written;
    }
    return written;
}

// --- 2. The Progress Bar (Visuals) ---
void progress_bar_loop() {
    while(total_downloaded_bytes < total_file_size) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        long long current;
        {
            std::lock_guard<std::mutex> lock(progress_mutex);
            current = total_downloaded_bytes;
        }

        double percent = (double)current / total_file_size * 100.0;
        if (percent > 100.0) percent = 100.0;

        std::cout << "\r[";
        int barWidth = 50;
        int pos = barWidth * percent / 100;
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "#";
            else std::cout << " ";
        }
        std::cout << "] " << std::fixed << std::setprecision(1) << percent << "% " << std::flush;
        
        if (current >= total_file_size) break;
    }
    std::cout << std::endl;
}

// --- 3. The Worker Thread (Downloads ONE chunk) ---
void download_chunk(int id, std::string url, long start, long end) {
    CURL* curl = curl_easy_init();
    std::string filename = "part_" + std::to_string(id);
    
    if(curl) {
        std::ofstream outfile(filename, std::ios::binary);
        
        // Define the Range (e.g., "0-1000")
        std::string range = std::to_string(start) + "-" + std::to_string(end);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outfile);
        curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str()); // <--- MAGIC HAPPENS HERE
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        curl_easy_perform(curl);
        outfile.close();
        curl_easy_cleanup(curl);
    }
}

// --- 4. Get File Size Helper ---
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

// --- 5. Merge Function ---
void merge_files(int num_threads, std::string final_name) {
    std::cout << "Merging files..." << std::endl;
    std::ofstream outfile(final_name, std::ios::binary);
    for(int i = 0; i < num_threads; i++) {
        std::string part_name = "part_" + std::to_string(i);
        std::ifstream infile(part_name, std::ios::binary);
        outfile << infile.rdbuf();
        infile.close();
        remove(part_name.c_str()); // Cleanup
    }
    outfile.close();
    std::cout << "Success! Saved as: " << final_name << std::endl;
}

int main(int argc, char* argv[]) {
    std::string url = "https://rr1---sn-3xoxu-ocvl.googlevideo.com/videoplayback?expire=1768859633&ei=kVNuaeezNtKE0u8P59WZoA4&ip=197.136.208.10&id=o-AONZ8hh0LqCrFMxBqDiXUDDxUSe19zx5jJHs3UzBAiZg&itag=18&source=youtube&requiressl=yes&xpc=EgVo2aDSNQ%3D%3D&cps=28&met=1768838033%2C&mh=F7&mm=31%2C29&mn=sn-3xoxu-ocvl%2Csn-woc7knez&ms=au%2Crdu&mv=m&mvi=1&pl=24&rms=au%2Cau&gcr=ke&initcwndbps=346250&bui=AW-iu_pXZG9VGKRe-mApLlcLW30hVr6FAXL2y9HOPcnrjaOIfZV-nZYWq1pKmoUsTEuzF8ul-2A-q-Mr&spc=q5xjPHHJHVWg2QMgWDjV&vprv=1&svpuc=1&xtags=heaudio%3Dtrue&mime=video%2Fmp4&rqh=1&cnr=14&ratebypass=yes&dur=398.036&lmt=1755168955904638&mt=1768837507&fvip=5&fexp=51552689%2C51565116%2C51565681%2C51580968&c=ANDROID&txp=4538534&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cxpc%2Cgcr%2Cbui%2Cspc%2Cvprv%2Csvpuc%2Cxtags%2Cmime%2Crqh%2Ccnr%2Cratebypass%2Cdur%2Clmt&sig=AJfQdSswRAIgfZiCeNLteJl1tayOiQBEcLFJ81FYfcREJf9AEp20H5UCIG07QLE8_35eK4ZUR6eeEKM48ueGQWVw0WaGL4Zv0Ok5&lsparams=cps%2Cmet%2Cmh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Crms%2Cinitcwndbps&lsig=APaTxxMwRQIhALngQh3ugnbDVYfG5vhFg-qmNEcckyOFM12BiItGJ-gUAiADMOvKFtfc-IdMjRis33vV-QAy5Ar4d-50vwIZzzndGA%3D%3D";
    
    // Allow user to override URL
    if(argc > 1) url = argv[1];

    total_file_size = (long)get_size(url);
    int num_threads = 4; // <--- THIS IS WHERE WE SET 4 THREADS

    std::cout << "Downloading " << total_file_size / (1024*1024) << " MB using " << num_threads << " threads..." << std::endl;

    // --- SPLIT THE FILE ---
    long chunk_size = total_file_size / num_threads;
    std::vector<std::thread> workers;

    // Launch threads
    for(int i = 0; i < num_threads; i++) {
        long start = i * chunk_size;
        long end = (i == num_threads - 1) ? total_file_size - 1 : (start + chunk_size - 1);
        workers.push_back(std::thread(download_chunk, i, url, start, end));
    }

    // Start UI
    progress_bar_loop();

    // Wait for threads
    for(auto& t : workers) t.join();

    // Combine parts
    merge_files(num_threads, "video.mp4");

    return 0;
}
