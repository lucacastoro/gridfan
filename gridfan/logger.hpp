#pragma once

#include <utility>
#include <iostream>
#include <syslog.h>

class Logger {
public:
  virtual ~Logger() = default;
  template <typename ...Args>
  inline void info(Args&&... args) {
    log(LOG_INFO, std::forward<Args>(args)...);
  }
  template <typename ...Args>
  inline void warning(Args&&... args) {
    log(LOG_WARNING, std::forward<Args>(args)...);
  }
  template <typename ...Args>
  inline void error(Args&&... args) {
    log(LOG_ERR, std::forward<Args>(args)...);
  }
private:
  virtual void log(int pri, const char* msg) = 0;
  template <typename ...Args>
  inline void log(int pri, const char* fmt, const Args& ...args) {
    char temp[1024];
    const auto l = snprintf(temp, sizeof(temp), fmt, args...);
    if (l > -1) {
      temp[l] = 0;
      log(pri, temp);
    }
  }
};

class SysLog final : public Logger {
public:
  SysLog() {
    openlog(nullptr, 0, 0);
  }
  virtual ~SysLog() override {
    closelog();
  }
private:
  void log(int pri, const char* msg) override {
    syslog(pri, "%s", msg);
  }
};

class LocalLog final : public Logger {
public:
  virtual ~LocalLog() override = default;
private:
  static inline const char* prefix(int pri) {
    return pri == LOG_ERR ?     "[ERROR]" :
           pri == LOG_WARNING ? "[WARN.]" :
           pri == LOG_INFO ?    "[INFO.]" :
                                "[?????]";
  }
  void log(int pri, const char* msg) override {
    std::cout << prefix(pri) << " " << msg << std::endl;
  }
};
