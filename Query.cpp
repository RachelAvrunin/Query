/* Rachel Mustakis 304976335 */
/* Avital Israeli 315842773 */

#include "Query.h"
#include "TextQuery.h"
#include <memory>
#include <set>
#include <algorithm>
#include <iostream>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <regex>
using namespace std;
////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<QueryBase> QueryBase::factory(const string& s)
{
  	string word1, word2;
    int wordNumber=0;
    bool NOTFlag=false;
    bool ANDFlag=false;
    bool ORFlag=false;
    bool NFlag=false;
    int n=0;
		string word;
    istringstream ist(s);
		while (ist >> word) {
      wordNumber++;

      if (wordNumber==1){                                       //first word

        if (word=="NOT")
          NOTFlag=true;
        else
          word1=word;
      }

      else if(wordNumber==2){                                   // second word

        if(NOTFlag)
          word1=word;
        else if(word=="AND")
          ANDFlag=true;
        else if(word=="OR")
          ORFlag=true;
        else if (!word.empty() && find_if(word.begin(), word.end(), [](char c) { return !isdigit(c); }) == word.end()){
          NFlag=true;
          int digit;
          for (int i=0 ; i<word.size() ; i++) {                 // for each char in word
            digit=word[i]-'0';
            n=(n*10)+digit;  
          }
        }
        else
          throw invalid_argument( "Unrecognized search" );
      }
      else if(wordNumber==3){                                   // third  word

        if(NOTFlag){
          throw invalid_argument( "Unrecognized search" ); 
        }
        else                                                  // if(ANDFlag || ORFlag || NFlag)
          word2=word;
      }
      else{                                                   // wordNumber==4 - > fourth  word
        throw invalid_argument( "Unrecognized search" );

      }
    }
    
    if (wordNumber==1)
      return std::shared_ptr<QueryBase>(new WordQuery(word1));
    if(wordNumber==2)
      return std::shared_ptr<QueryBase>(new NotQuery(word1));
    if(ANDFlag)
      return std::shared_ptr<QueryBase>(new AndQuery(word1,word2));
    if(ORFlag)
      return std::shared_ptr<QueryBase>(new OrQuery(word1,word2));
    if(NFlag)
      return std::shared_ptr<QueryBase>(new NQuery(word1,word2,n));
}

////////////////////////////////////////////////////////////////////////////////
QueryResult NotQuery::eval(const TextQuery &text) const
{
  QueryResult result = text.query(query_word);
  auto ret_lines = std::make_shared<std::set<line_no>>();
  auto beg = result.begin(), end = result.end();
  auto sz = result.get_file()->size();
  
  for (size_t n = 0; n != sz; ++n)
  {
    if (beg==end || *beg != n)
		ret_lines->insert(n);
    else if (beg != end)
		++beg;
  }
  return QueryResult(rep(), ret_lines, result.get_file());
    
}

QueryResult AndQuery::eval (const TextQuery& text) const
{
  QueryResult left_result = text.query(left_query);
  QueryResult right_result = text.query(right_query);
  
  auto ret_lines = std::make_shared<std::set<line_no>>();
  std::set_intersection(left_result.begin(), left_result.end(),
      right_result.begin(), right_result.end(), 
      std::inserter(*ret_lines, ret_lines->begin()));

  return QueryResult(rep(), ret_lines, left_result.get_file());
}

QueryResult OrQuery::eval(const TextQuery &text) const
{
  QueryResult left_result = text.query(left_query);
  QueryResult right_result = text.query(right_query);
  
  auto ret_lines = 
      std::make_shared<std::set<line_no>>(left_result.begin(), left_result.end());

  ret_lines->insert(right_result.begin(), right_result.end());

  return QueryResult(rep(), ret_lines, left_result.get_file());
}
/////////////////////////////////////////////////////////
QueryResult NQuery::eval(const TextQuery &text) const{
  QueryResult QR = AndQuery::eval(text);
  regex firstWord(left_query);
  regex secondWord(right_query);
  auto ret_lines = std::make_shared<std::set<line_no>>();//where we return our lines
  //Need regex to seperate the spaces between words and then I can count the number of words in between the 2 given words 
  //lets go over the lines:

  for ( auto itr = QR.begin(); itr != QR.end(); ++itr){
    string line = QR.get_file()->at(*itr);

    std::smatch match;
    regex_search(line, match, firstWord);
    int firstPos=match.position(0);// position of first word
    
    int i=firstPos + match.length();
    regex_search(line,match,secondWord);
    int secondPos=match.position(0);// position of    second word

    if(firstPos>secondPos){
      int j=firstPos;
      firstPos=secondPos;
      secondPos=j;
      regex_search(line, match, secondWord);
      i=firstPos + match.length();
    }

    int WordsBetween=0;
    bool isBlank = true;
    while(i < secondPos-1){
      if(isspace(line[i])){
        isBlank = true;
      }
      else if(isBlank){
        WordsBetween++;
        isBlank = false;
      }
      
      ++i;
    }
    if(WordsBetween <= dist){
      ret_lines->insert(*itr);
    }
  }
  return QueryResult(rep(), ret_lines, QR.get_file());
}
/////////////////////////////////////////////////////////