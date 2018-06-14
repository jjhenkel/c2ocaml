#pragma once

namespace c2ocaml {
namespace frontend {
namespace util {

// trim from start (in place)
inline void ltrim(std::string &s) {
  s.erase(s.begin(),
          std::find_if(s.begin(), s.end(),
                       std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end (in place)
inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       std::not1(std::ptr_fun<int, int>(std::isspace)))
              .base(),
          s.end());
}

// trim from both ends (in place)
inline void trim(std::string &s) {
  ltrim(s);
  rtrim(s);
}

inline bool fexists(const std::string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

template<class OutIter>
inline OutIter write_escaped(std::string const& s, OutIter out) {
  *out++ = '"';
  for (std::string::const_iterator i = s.begin(), end = s.end(); i != end; ++i) {
    unsigned char c = *i;
    if (' ' <= c and c <= '~' and c != '\\' and c != '"') {
      *out++ = c;
    }
    else {
      *out++ = '\\';
      switch(c) {
      case '"':  *out++ = '"';  break;
      case '\\': *out++ = '\\'; break;
      case '\t': *out++ = 't';  break;
      case '\r': *out++ = 'r';  break;
      case '\n': *out++ = 'n';  break;
      default:
        char const* const hexdig = "0123456789ABCDEF";
        *out++ = 'x';
        *out++ = hexdig[c >> 4];
        *out++ = hexdig[c & 0xF];
      }
    }
  }
  *out++ = '"';
  return out;
}

inline void str_replace_all(std::string &str, const std::string& from, const std::string& to) {
  size_t start_pos = 0;
  while((start_pos = str.find(from, start_pos)) != std::string::npos) {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
  }
}

inline void str_replace(std::string &str, const std::string &from,
                        const std::string &to) {
  auto start_pos = str.find(from);

  if (start_pos == std::string::npos)
    return;

  str.replace(start_pos, from.length(), to);
}

inline std::string str_numeric_only(std::string &str) {
  str.erase(std::remove_if(str.begin(), str.end(), (int (*)(int))std::isalpha),
            str.end());
  // std::cerr << str << " @(" << local << ")" << std::endl;
  return str;
}

inline std::string cwd() {
  char buff[FILENAME_MAX];

  getcwd(buff, FILENAME_MAX);

  std::string res = std::string(buff);
  trim(res);
  return res;
}

inline std::string time_stamp_as_str() {
  const time_t nowraw = time(0);
  struct tm *now;
  char buffer[30];
  now = localtime(&nowraw);
  strftime(buffer, sizeof(buffer), "%H:%M:%S %m/%d/%Y", now);
  return std::string(buffer);
}

inline std::string repo_cwd() {
  auto the_cwd = cwd();
  str_replace(the_cwd, REPO_ROOT, "#");
  return the_cwd;
}

inline bool path_not_in_repo(const std::string &path) {
  // Skip empty ones
  if (path.length() <= 0) {
    std::cerr << "WARNING: empty path" << std::endl;
    return true;
  }

  // Things MUST BE IN /app/tst
  char resolved_path[PATH_MAX];
  realpath(path.c_str(), resolved_path);

  std::string rel_path(resolved_path);
  str_replace(rel_path, "/app", "#");
  return rel_path[0] != '#';
}
}
}
} // c2ocaml::frontend::util
