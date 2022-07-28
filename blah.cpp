#include <chrono>
#include <thread>
#include <sys/ptrace.h>
#include <cstring>
#include <cxxabi.h>
#include <unistd.h>
#include <libunwind-ptrace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/wait.h>

using namespace std::chrono_literals;

void OtherFunction() {
  std::this_thread::sleep_for(2000ms);
}

void CallOtherFunction() {
  ptrace(PTRACE_TRACEME, 0, NULL, NULL);
  OtherFunction();
}

void CalcHashesRemote(pid_t);

int main(int, char**) {
  pid_t child_pid = fork();
  if (child_pid == 0) {
    CallOtherFunction();
    _exit(0);
  }

  ptrace(PTRACE_ATTACH, child_pid, 0, 0);

  waitpid(child_pid, NULL, 0);

  printf("Here we go\n");
  CalcHashesRemote(child_pid);
}

// https://github.com/daniel-thompson/libunwind-examples/blob/master/unwind-pid.c
void CalcHashes(unw_cursor_t* cursor, const char* skip_until_past) {
  std::string major_stack, minor_stack;

  char sym[4096];
  size_t count = 0;
  // default case is that we already saw the skip_until_past and there's
  // nothing that needs to be skipped.
  bool saw_skip_until = (skip_until_past == nullptr);
  do {
    count++;
    unw_word_t offset;
    unw_word_t pc;

    if (unw_get_reg(cursor, UNW_REG_IP, &pc)) {
      return;
    }

    int res = unw_get_proc_name(cursor, sym, sizeof(sym), &offset);
    if (res == 0 || res == UNW_ENOMEM) {
      int status;
      size_t demangled_size;
      char* demangled = abi::__cxa_demangle(sym, NULL, &demangled_size, &status);
      if (demangled != NULL) {
        snprintf(sym, sizeof(sym), "%s+0x%lx", demangled, offset);
        free(demangled);
      } else {
        snprintf(sym + strlen(sym), sizeof(sym) - strlen(sym), "+0x%lx", offset);
      }
    } else {
      snprintf(sym, sizeof(sym), "??");
    }

    if (skip_until_past != nullptr) {
      // always skip the skip_until_past
      if (strstr(sym, skip_until_past) != nullptr) {
        saw_skip_until = true;
        continue;
      // keep skipping until we've seen the skip until past
      } else if (!saw_skip_until) {
        continue;
      }
    }

    if (count <= 5) {
      if (count > 0) { major_stack += "\n"; }
      major_stack += sym;
    }
    if (count > 0) { minor_stack += "\n"; }
    minor_stack += sym;

    if (strstr(sym, "LLVMFuzzerTestOneInput") != NULL) {
      break;
    }
  } while (unw_step(cursor) > 0);

  printf("MINOR STACK:\n%s\n", minor_stack.c_str());
}

void CalcHashesRemote(pid_t child_pid) {
  unw_addr_space_t as = unw_create_addr_space(&_UPT_accessors, 0);

  void* context = _UPT_create(child_pid);
  unw_cursor_t cursor;
  int err;
  if ((err = unw_init_remote(&cursor, as, context)) != 0) {
    if (err == -UNW_EINVAL) {
      printf("UNW_EINVAL\n");
    } else if (err == -UNW_EUNSPEC) {
      printf("UNW_EUNSPEC\n");
    } else if (err == -UNW_EBADREG) {
      printf("could not init remote: UNW_EBADREG\n");
    }
    _UPT_destroy(context);
    free(as);
    return;
  }

  CalcHashes(&cursor, nullptr);

  _UPT_destroy(context);
  free(as);
}

