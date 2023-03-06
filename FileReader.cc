/*
 * Copyright ©2022 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to the students registered for University of Pennsylvania
 * CIT 595 for use solely during Spring Semester 2022 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <stdio.h>
#include <string>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



#include "./HttpUtils.h"
#include "./FileReader.h"

using std::string;

namespace searchserver {

bool FileReader::read_file(string *str) {
  // Read the file into memory, and store the file contents in the
  // output parameter "str."  Be careful to handle binary data
  // correctly; i.e., you probably want to use the two-argument
  // constructor to std::string (the one that includes a length as a
  // second argument).

  // TODO: implement
  string result_string = string();
  const string& fname = this->fname_;



  int fd = open(fname.c_str(), O_RDONLY);
  if(fd==-1){
    return false;
  } 
  int result;
  char buf;
  while(true){
    result = read(fd, &buf, 1);
    if(result==-1){
      if(errno!=EINTR){
        // this->good_ = false;
        exit(EXIT_FAILURE);
      }
      continue;
    }else if(result ==0){
      break;
    }
    result_string += buf;
  }
  *str = result_string;
  close(fd);
  return true;
}

}  // namespace searchserver
