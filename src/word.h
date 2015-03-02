using namespace std;

#include <boost/algorithm/string.hpp>
using namespace boost::algorithm;







class Word {
  
public:

  virtual void execute() = 0;
  virtual string name() = 0;

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

  virtual bool equals(Word * word) {
    if (word == this) {
      return true;
    } else {
      return false;
    }
  }

  virtual bool as_bool() {
    return true;
  }
  virtual int as_int() {
    return 1;
  }

  virtual string as_string() {
    return name();
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

  static IntegerWord * parse(string token) {
    return new IntegerWord(stoi(token));
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
  void execute() {
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
  
  virtual bool as_bool() {
    if (n == 0) {
      return false;
    } else {
      return true;
    }
  }
  virtual int as_int() {
    return n;
  }
};


class StringWord: public Word
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

  static StringWord * parse(string token) {
    if (token[0] == '\"' && token[token.size() - 1] == '\"') {
      return new StringWord(token.substr(1, token.size() - 2));      
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
  void execute() {
  }

  string name() {
    stringstream ss;
    ss << "\"" << s << "\"";
    return ss.str();
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

  static SymbolWord * parse(string token) {
    if (token.size() != 0) {
      return new SymbolWord(token);
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
  void execute() {
  }

  string name() {
    stringstream ss;
    ss << "'" << s;
    return ss.str();
  }
  
};


class CompoundWord {
 private:
  vector<Word *> stack;
 public:
  CompoundWord(vector<Word *> _stack) {
    stack = _stack;
  }
  virtual void execute();
};


std::map<int, Word *> create_character_code_to_word(std::vector<Word *> words);
std::map<string, Word *> create_name_to_word(std::vector<Word *> words);
std::vector<Word *> create_words();
