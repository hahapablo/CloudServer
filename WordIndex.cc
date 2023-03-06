#include "./WordIndex.h"
#include "./FileReader.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
namespace searchserver {

WordIndex::WordIndex() {
  // TODO: implement
}

size_t WordIndex::num_words() {
  // TODO: implement
  return this->table.size();
}

void WordIndex::record(const string& word, const string& doc_name) {
  // TODO: implement
  if(std::find(this->docs.begin(), this->docs.end(), doc_name) == this->docs.end()){
    this->docs.push_back(doc_name);
  }
  if(this->table.find(word)!= this->table.end()){
    if(this->table[word].find(doc_name)!=this->table[word].end()){
      this->table[word][doc_name]+=1;
    }else{
      this->table[word][doc_name]=1;
    }
  }else{
    map<string,int> temp;
    temp[doc_name] = 1;
    this->table[word] = temp;
  }
  // FileReader f(doc_name);
  // string contents;
  // vector<string> strs;
  // boost::split(strs, contents, boost::is_any_of("\n"));
  // boost::split(strs, contents, boost::is_any_of("\t"));
  // boost::split(strs, contents, boost::is_any_of(" "));
  // int counter = 0;
  // for (size_t i = 0; i < strs.size(); i++){
  //   // std::cout<<"----- "<<strs[i]<<std::endl;
  //   if(strs[i]==word){
  //     counter++;
  //   }
  // }
  // this->table[word][doc_name] = 0;
}

list<Result> WordIndex::lookup_word(const string& word) {
  list<Result> result;
  // TODO: implement
  map<string, int> counter = this->table[word];
  for (auto const& [key, val] : counter){
    Result temp = Result(key,val);
    result.push_back(temp);
  }
  result.sort();
  return result;
}

  // Lookup a query (multiple words) in the index, getting a sortede list of all documents
  // that contain each word in the query and a rank which is the number of occurances
  // of each word in the document.
  //
  // Arguments:
  //  - word: a word we are looking up results for
  //
  // Returns:
  //  - A list of results. Each result contains a document name and the sum of the
  //    number of recorded occurances of the each query word in that document. The list is
  //    sorted with documents with the highest rank at the front.
list<Result> WordIndex::lookup_query(const vector<string>& query) {
  list<Result> results;
  map<string, int> counter;
  for (auto const& doc : this->docs){
    bool all_found = true;
    int temp_n = 0;
    for(auto const& word : query){
      if(all_found==false || this->table[word].find(doc)==this->table[word].end()){
        all_found = false;
        continue;
      }
      temp_n+=this->table[word][doc];
    }
    if(all_found && temp_n>0){
      Result temp = Result(doc,temp_n);
      results.push_back(temp);
    }
  }
  results.sort();
  return results;
}

}  // namespace searchserver
