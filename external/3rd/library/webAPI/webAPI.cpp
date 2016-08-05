/*
This code is just a simple wrapper around nlohmann's wonderful json lib
(https://github.com/nlohmann/json) and libcurl. While originally included directly,
we have come to realize that we may require web API functionality elsewhere in the future.

As such, and in an effort to keep the code clean, we've broken it out into this simple little
namespace/lib that is easy to include. Just make sure to link against curl when including, and
make all the cmake modifications required to properly use it.

(c) stellabellum/swgilluminati (combined crews), written by DA with help from DC
based on the original prototype by parz1val

License: what's a license? we're a bunch of dirty pirates!
*/

#include "webAPI.h"

using namespace std;

/*
endpoint: URI
data: get/post
messageSlot: key for fetching a verbose user friendly message
statusSlot: slot containing the success/fail value
statusVal: the expected value in the case of success
*/
webAPI::statusMessage webAPI::simplePost(const string &endpoint, const string &data, const string &slotName, const string &messageSlot, const string &statusSlot, const string &statusVal)
{
	// declare our output and go ahead and attempt to get data from remote
	nlohmann::json response = request(endpoint, data, 1);

	// if we got data back...
	if (response.count(statusSlot) && response[statusSlot].get<std::string>() == statusVal && response.count(slotName))
	{
		return {true, response[slotName].get<std::string>()};
		
	}
	else //default message is an error, the other end always assumes "success" or the specified slot
	{
		if (response.count(messageSlot))
		{
			return {false, response[messageSlot].get<std::string>()};
		}
	}

	return {false, "Message not provided by remote."};
}

// this can be broken out to separate the json bits later if we need raw or other http type requests
// all it does is fetch via get or post, and if the status is 200 returns the json, else error json
nlohmann::json webAPI::request(const string &endpoint, const string &data, const int &reqType)
{
	nlohmann::json response;

	if (!endpoint.empty()) //data is allowed to be an empty string if we're doing a normal GET
	{
		CURL *curl = curl_easy_init(); // start up curl

		if (curl)
		{
			string readBuffer; // container for the remote response
			long http_code = 0; // we get this after performing the get or post

			curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str()); // endpoint is always specified by caller
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback); // place the data into readBuffer using writeCallback
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer); // specify readBuffer as the container for data

			if (reqType == 1)
			{
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
			} //todo - get request? etc

			CURLcode res = curl_easy_perform(curl); // make the request!

			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code); //get status code

			if (res == CURLE_OK && http_code == 200 && !(readBuffer.empty())) // check it all out and parse
			{
				try {
					response = nlohmann::json::parse(readBuffer);
				}
				catch (string &e) {
					response["message"] = e;
					response["status"] = "failure";
				}
				catch (...) {
					response["message"] = "JSON parse error for endpoint.";
					response["status"] = "failure";
				}
			}
			else
			{
				response["message"] = "Error fetching data from remote.";
				response["status"] = "failure";
			}
			curl_easy_cleanup(curl); // always wipe our butt
		}
		else //default err messages below
		{
			response["message"] = "Failed to initialize cURL.";
			response["status"] = "failure";
		}
	}
	else
	{
		response["message"] = "Invalid endpoint URL.";
		response["status"] = "failure";
	}

	return response;
}

// This is used by curl to grab the response and put it into a var
size_t webAPI::writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}
