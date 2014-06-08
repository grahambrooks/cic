#pragma once

#include <curl/curl.h>
#include <iostream>

using namespace std;
namespace cic {
    namespace HTTP {
        class client {
        public:
            virtual string get(string url, string username, string password) = 0;
        };

        class curl_client : public client {
        private:
            string collector;
        public:

            static size_t callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
                curl_client *handler = static_cast<curl_client *>(userdata);

                handler->collector += string(ptr, size * nmemb);
                return size * nmemb;
            }


            string get(string url, string username = "", string password = "") {
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
