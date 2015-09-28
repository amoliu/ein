#ifndef _WORD_H_
#define _WORD_H_

#include "ein_util.h"

using namespace std;
#include <sstream>
#include <map>
#include <boost/algorithm/string.hpp>

class MachineState;

using namespace boost::algorithm;

class Word {
  
public:

  virtual void execute(std::shared_ptr<MachineState> ms);
  virtual string name() = 0;

  virtual string description() {
    return "No help for this word.";
  }

  virtual vector<string> names() {
    vector<string> result;
    result.push_back(name());
    return result;
  }

  virtual bool is_value() {
    return false;
  }
     

  virtual int character_code() {
    return -1;
  }

  virtual bool equals(std::shared_ptr<Word> word) {
    if (word.get() == this) {
      return true;
    } else {
      return false;
    }
  }

  virtual bool to_bool() {
    return true;
  }
  virtual int to_int() {
    return 1;
  }

  /**
   * Returns it as a string in the repl (e.g., quotes, back ticks for strings and symbols)
   */
  virtual string repr() {
    return name();
  }

  /**
   * Converts to a string (e.g., no quotes)
   */
  virtual string to_string() {
    return name();
  }

};

class DoubleWord: public Word
{
private:
  double n;

public:

  double value() {
    return n;
  }

  virtual bool is_value() {
    return true;
  }

  static std::shared_ptr<DoubleWord> parse(string token) {
    char* endptr;
    //double r = strtod(token.c_str(), &endptr); 
    double r = stod(token); 
    /* try to do this with exceptions
    if (endptr == token && r == 0) { 
      // failed to convert
      cout << "double parse failed to convert" << endl;
      ms->pushWord("pauseStackExecution");
      return;
    }
    */
    return std::make_shared<DoubleWord>(r);
  }
  virtual bool is_static() {
    return false;
  }
  static bool isDouble(string token) {
    try {
      parse(token);
      return true;
    } catch (...) {
      return false;
    }
  }
  
  DoubleWord(double _n) {
    n = _n;
  }
  string name() {
    stringstream ss;
    ss << scientific;
    ss << n;
    return ss.str();
  }

  bool equals(Word * word) {
    DoubleWord * w1 = dynamic_cast<DoubleWord *>(word);
    if (w1 == NULL) {
      return false;
    } else {
      return w1->value() == this->value();
    }
  }
  
  virtual bool to_bool() {
    if (n == 0) {
      return false;
    } else {
      return true;
    }
  }
  virtual double to_double() {
    return n;
  }
};


class IntegerWord: public Word
{
private:
  int n;

public:

  int value() {
    return n;
  }

  virtual bool is_value() {
    return true;
  }

  static std::shared_ptr<IntegerWord> parse(string token) {
    size_t idx;
    int i = stoi(token, &idx);
    if (idx != token.size()) {
      throw 7;
    } else {
      return std::make_shared<IntegerWord>(i);
    }
  }
  virtual bool is_static() {
    return false;
  }
  static bool isInteger(string token) {
    try {
      parse(token);
      return true;
    } catch (...) {
      return false;
    }
  }
  
  IntegerWord(int _n) {
    n = _n;
  }
  string name() {
    stringstream ss;
    ss << n;
    return ss.str();
  }

  bool equals(Word * word) {
    IntegerWord * w1 = dynamic_cast<IntegerWord *>(word);
    if (w1 == NULL) {
      return false;
    } else {
      return w1->value() == this->value();
    }
  }
  
  virtual bool to_bool() {
    if (n == 0) {
      return false;
    } else {
      return true;
    }
  }
  virtual int to_int() {
    return n;
  }
};


class StringWord: public Word
{
private:
  string s;

public:

  string value() {
    return s;
  }

  virtual bool is_value() {
    return true;
  }

  virtual bool is_static() {
    return false;
  }

  static std::shared_ptr<StringWord> parse(string token) {
    if (token[0] == '\"' && token[token.size() - 1] == '\"') {
      return std::make_shared<StringWord>(token.substr(1, token.size() - 2));      
    } else {
      return NULL;
    }

  }
  static bool isString(string token) {
    try {
      if (parse(token) != NULL) {
        return true;
      } else {
        return false;
      }
    } catch (...) {
      return false;
    }
  }
  
  StringWord(string _s) {
    s = _s;
  }

  string name() {
    stringstream ss;
    ss << "\"" << s << "\"";
    return ss.str();
  }

  string to_string() {
    return s;
  }
  
};


class SymbolWord: public Word
{
private:
  string s;

public:

  virtual bool is_value() {
    return true;
  }

  virtual bool is_static() {
    return false;
  }

  static std::shared_ptr<SymbolWord> parse(string token) {
    if (token.size() != 0) {
      return std::make_shared<SymbolWord>(token);
    } else {
      return NULL;
    }
  }
  static bool isSymbol(string token) {
    if (! IntegerWord::isInteger(token) && ! StringWord::isString(token)) {
      return true;
    } else {
      return false;
    }
  }
  
  SymbolWord(string _s) {
    s = _s;
  }

  string name() {
    stringstream ss;
    ss << "'" << s;
    return ss.str();
  }

  string to_string() {
    return s;
  }
  
};

class EePoseWord: public Word
{
private:
  eePose pose;

public:
  eePose value() {
    return pose;
  }

  virtual bool is_value() {
    return true;
  }

  static std::shared_ptr<EePoseWord> parse(string token) {
    
    eePose pose;
    return std::make_shared<EePoseWord>(pose);
  }
  virtual bool is_static() {
    return false;
  }
  static bool isInteger(string token) {
    if (token.substr(0,5) == "eePose") {
      return true;
    } else {
      return false;
    }
  }
  
  EePoseWord(eePose _pose) {
    pose = _pose;
  }
  string name() {
    stringstream ss;
    ss << pose;
    return ss.str();
  }

  bool equals(Word * word) {
    EePoseWord * w1 = dynamic_cast<EePoseWord *>(word);
    if (w1 == NULL) {
      return false;
    } else {
      return w1->value().equals(this->value());
    }
  }
  
  virtual bool to_bool() {
    return true;
  }
  virtual int to_int() {
    return 1;
  }
};




class CompoundWord {
 private:
  vector<std::shared_ptr<Word> > stack;
 public:
  CompoundWord(vector<std::shared_ptr<Word> > _stack) {
    stack = _stack;
  }
};


std::map<int, std::shared_ptr<Word> > create_character_code_to_word(std::vector<std::shared_ptr<Word> > words);
std::map<string, std::shared_ptr<Word> > create_name_to_word(std::vector<std::shared_ptr<Word> > words);

std::shared_ptr<Word> parseToken(std::shared_ptr<MachineState> ms, string token);
std::shared_ptr<Word> nameToWord(string name);

extern std::vector<std::shared_ptr<Word> > words;
extern std::map<int, std::shared_ptr<Word> > character_code_to_word;
extern std::map<string, std::shared_ptr<Word> > name_to_word;

void initializeWords();

void renderCoreView(shared_ptr<MachineState> ms);

/*
WORD()
virtual void execute(std::shared_ptr<MachineState> ms)
{
    ms->pushWord("");
}
END_WORD
REGISTER_WORD()
*/

#endif /* _WORD_H_ */
