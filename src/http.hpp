#include <curl/curl.h>
#include <iostream>

namespace ci {
    namespace HTTP {
        class client {
            virtual std::string get(std::string url, std::string username, std::string password) = 0;
        };
        
        class curl_client : public client {
        private:
            std::string collector;
        public:
            
            static size_t callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
                curl_client *handler = static_cast<curl_client *>(userdata);
                
                handler->collector += std::string(ptr, size * nmemb);
                return size * nmemb;
            }
            
            
            std::string get(std::string url, std::string username = "", std::string password = "") {
                collector.clear();
                CURL *curl;
                CURLcode res;
                
                curl = curl_easy_init();
                if (curl) {
                    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                    
                    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                    
                    if (username.length() > 0 || password.length() > 0) {
                        curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
                        curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
                    }
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &callback);
                    
                    res = curl_easy_perform(curl);
                    
                    if (res != CURLE_OK) {
                        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                    }
                    
                    curl_easy_cleanup(curl);
                    
                }
                
                return collector;
            }
        };
    }
}