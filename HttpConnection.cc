/*
 * Copyright ©2022 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Pennsylvania
 * CIT 595 for use solely during Spring Semester 2022 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <cstdint>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>
#include <vector>

#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpConnection.h"

using std::map;
using std::string;
using std::vector;

namespace searchserver {

static const char *kHeaderEnd = "\r\n\r\n";
static const int kHeaderEndLen = 4;


// Read and parse the next request from the file descriptor fd_,
// storing the state in the output parameter "request."  Returns
// true if a request could be read, false if the parsing failed
// for some reason, in which case the caller should close the
// connection.

bool HttpConnection::next_request(HttpRequest *request) {
  // Use "wrapped_read" to read data into the buffer_
  // instance variable.  Keep reading data until either the
  // connection drops or you see a "\r\n\r\n" that demarcates
  // the end of the request header.
  //
  // Once you've seen the request header, use parse_request()
  // to parse the header into the *request argument.
  //
  // Very tricky part:  clients can send back-to-back requests
  // on the same socket.  So, you need to preserve everything
  // after the "\r\n\r\n" in buffer_ for the next time the
  // caller invokes next_request()!

  // TODO: implement
  size_t ptr;
  while (1) {
    ptr = buffer_.find(kHeaderEnd, 0, 4);
    if (ptr != string::npos) {
      break;
    }
    wrapped_read(fd_, &buffer_);
  }

  string new_req = buffer_.substr(0, ptr + 4);
  
  // chop off the first section
  buffer_ = buffer_.substr(ptr + 4, string::npos);

  return parse_request(new_req, request);
}


// Write the response to the file descriptor fd_.  Returns true
// if the response was successfully written, false if the
// connection experiences an error and should be closed.
bool HttpConnection::write_response(const HttpResponse &response) {
  // Implement so that the response is converted to a string
  // and written out to the socket for this connection

  // TODO: implement
  const string resp = response.GenerateResponseString();
  int signal = wrapped_write(fd_, resp);
  if (signal >= 0) {
    return true;
  }
  return false;
}

// A helper function to parse the contents of data read from
// the HTTP connection. Returns true if the request was parsed
// successfully and that request is returned through *request
// and returns false if the Request is an invalid format
bool HttpConnection::parse_request(const string &request, HttpRequest* out) {
  HttpRequest req("/");  // by default, get "/".

  // Split the request into lines.  Extract the URI from the first line
  // and store it in req.URI.  For each additional line beyond the
  // first, extract out the header name and value and store them in
  // req.headers_ (i.e., HttpRequest::AddHeader).  You should look
  // at HttpRequest.h for details about the HTTP header format that
  // you need to parse.
  //
  // You'll probably want to look up boost functions for (a) splitting
  // a string into lines on a "\r\n" delimiter, (b) trimming
  // whitespace from the end of a string, and (c) converting a string
  // to lowercase.
  //
  // If a request is malfrormed, return false, otherwise true and 
  // the parsed request is retrned via *out
  
  // TODO: implement
  vector<string> lines;
  boost::split(lines, request, boost::is_any_of("\r\n"));
  if (lines.size() == 0) {
    return false;
  }
  
  vector<string> components;
  string firstLine = lines[0];
  boost::split(components, firstLine, boost::is_any_of(" "), boost::token_compress_on);
  if (components.size() < 3) {
    return false;
  }
  req.set_uri(components[1]);

  for (size_t i = 1; i<lines.size(); i++) {
    if (lines[i].empty()) {
      continue;
    }
    vector<string> pair;
    boost::split(pair, lines[i], boost::is_any_of(":"), boost::token_compress_on);
    if (pair.size()<=1) {
      return false;
    }
    for (auto& each: pair) {
      boost::trim(each);
      boost::to_lower(each);
    }
    string key = pair[0];
    string value = pair[1];
    req.AddHeader(key, value);
  }
  *out = req;
  return true;
}

}  // namespace searchserver
