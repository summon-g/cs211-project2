/*main.cpp*/

/**
 * @brief simple debugger for nuPython programs.
 *
 * To run:
 *
 *   ./a.out filename.py
 *
 * @author original template by Prof. Joe Hummel
 * @author Carson Apple
 * @author Charlie Du
 * @author Summon Gebrelibanos
 * @note Northwestern University
 */

#include <iostream>
#include <set>
#include <string>

using std::cin;
using std::cout;
using std::endl;
using std::getline;
using std::set;
using std::stoi;
using std::string;

#include "pgutil.h"

//
// Since we are working in C++, we have to tell the
// C++ compiler that the functions in the provided
// nuPython code are C functions:
//
extern "C"
{
#include "execute.h"
#include "parser.h"
#include "programgraph.h"
#include "ram.h"
#include "tokenqueue.h"
}

/**
 * @brief help command
 *
 * @return void
 */
void help_command()
{
  cout << "Available commands:" << endl;
  cout << "h -> help with list of commands" << endl;
  cout << "q -> quit the debugger" << endl;
  cout << "b n -> set breakpoint at line n" << endl;
  cout << "rb n -> remove breakpoint at line n" << endl;
  cout << "cb -> clear all breakpoints" << endl;
  cout << "lb -> list all breakpoints" << endl;
  cout << "pg -> print entire program graph" << endl;
  cout << "r -> run the program from the current stmt" << endl;
  cout << "s -> step by executing the current stmt" << endl;
  cout << "sm -> show memory contents" << endl;
  cout << "w -> what line will execute next?" << endl;
}

/**
 * @brief print program graph command
 *
 * @return void
 */
void print_graph_command(struct STMT *program)
{
  programgraph_print(program);
}

/**
 * @brief show memory contents command
 *
 * @return void
 */
void show_memory_command(struct RAM *memory)
{
  ram_print(memory);
}

/**
 * @brief set breakpoint at line n command
 *
 * @param n line number to set the breakpoint at
 * @return void
 */
void breakpoint_command(set<int> &breakpoints, int n)
{
  if (breakpoints.count(n))
    cout << "breakpoint exists" << endl;
  else
  {
    breakpoints.insert(n);
    cout << "breakpoint set" << endl;
  }
}

/**
 * @brief remove breakpoint at line n command
 *
 * @param n line number to remove the breakpoint from
 * @return void
 */
void remove_breakpoint_command(set<int> &breakpoints, int n)
{
  if (!breakpoints.count(n))
  {
    cout << "no such breakpoint" << endl;
  }
  else
  {
    breakpoints.erase(n); // remove breakpoint
    cout << "breakpoint removed" << endl;
  }
}

/**
 * @brief clear all breakpoint command
 *
 * @return void
 */
void clear_breakpoint_command(set<int> &breakpoints)
{
  breakpoints.clear(); // remove all elements from set<int> breakpoints
  cout << "all breakpoints removed" << endl;
}

/**
 * @brief list breakpoint command
 *
 * @return void
 */
void list_breakpoint_command(set<int> &breakpoints)
{
  if (breakpoints.empty())
    cout << "breakpoints: none" << endl;
  else
  {
    cout << "breakpoints:"; // print this once
    for (int i : breakpoints)
    {
      if (i != *breakpoints.begin())
      {
        cout << ",";
      }
      cout << " " << i;
    }
    cout << endl;
  }
}

/**
 * @brief what line will execute next command
 *
 * @return void
 */
void where_command(struct STMT *cur)
{
  if (cur == nullptr)
  {
    cout << "execution has completed" << endl;
    return;
  }

  // break the graph so only cur gets printed:
  struct PG_LINKS links = break_pg(cur);
  programgraph_print(cur);
  repair_pg(cur, links);
}

/**
 * @brief executes cur and sets next to the stmt that
 *        should run after it
 *
 * @return true if it worked, false on a semantic error
 */
bool execute_stmt(struct STMT *cur, struct RAM *memory, struct STMT *&next)
{
  //
  // while loop: just check the condition:
  //
  if (cur->stmt_type == STMT_WHILE_LOOP)
  {
    struct RAM_VALUE *result =
        execute_expr(cur, memory, cur->types.while_loop->condition);

    if (result == nullptr) // condition eval failed
      return false;

    if (result->types.i != 0)
      next = cur->types.while_loop->loop_body;
    else
      next = cur->types.while_loop->next_stmt;

    ram_free_value(result);
    return true;
  }

  //
  // if stmt: just check the condition:
  //
  if (cur->stmt_type == STMT_IF_THEN_ELSE)
  {
    struct RAM_VALUE *result =
        execute_expr(cur, memory, cur->types.if_then_else->condition);

    if (result == nullptr) // condition eval failed
      return false;

    if (result->types.i != 0)
      next = cur->types.if_then_else->true_path;
    else
      next = cur->types.if_then_else->false_path;

    ram_free_value(result);
    return true;
  }

  //
  // everything else: break the graph, run just cur, repair:
  //
  struct PG_LINKS links = break_pg(cur);
  struct ExecuteResult result = execute(cur, memory);
  repair_pg(cur, links);

  if (!result.Success) // error message already printed
    return false;

  next = links.next;
  return true;
}

/**
 * @brief step by executing the current statement command
 *
 * @return true if we can keep going, false if not
 */
bool step_command(struct STMT *&cur, struct RAM *memory,
                  set<int> &breakpoints, bool &at_breakpoint,
                  bool consume_input = true)
{
  if (cur == nullptr)
  {
    cout << "execution has completed" << endl;
    return false;
  }

  // stop at a breakpoint, unless we already stopped here:
  if (!at_breakpoint && breakpoints.count(cur->line) > 0)
  {
    at_breakpoint = true;
    cout << "breakpoint hit at line " << cur->line << endl;
    return false;
  }

  // eat the rest of the line in case cur calls input():
  if (consume_input)
  {
    string rest;
    getline(cin, rest);
  }

  struct STMT *next = nullptr;

  if (!execute_stmt(cur, memory, next))
    return false; // leave cur at the stmt that failed

  cur = next;
  at_breakpoint = false;

  if (cur == nullptr)
  {
    cout << "program completed" << endl;
    return false;
  }

  return true;
}

/**
 * @brief run program from current statement command
 *
 * @return void
 */
void run_command(struct STMT *&cur, struct RAM *memory,
                 set<int> &breakpoints, bool &at_breakpoint)
{
  // eat the rest of the line once before running:
  string rest;
  getline(cin, rest);

  // keep stepping until done, error, or breakpoint:
  while (step_command(cur, memory, breakpoints, at_breakpoint, false))
  {
  }
}

/**
 * @brief main
 *
 * usage: ./a.out filename.py
 *
 * If a filename is given, the file is opened and serves as
 * input to the debugger.
 *
 * @param argc # of arguments to program
 * @param argv Array of actual arguments
 * @return 0 Success
 */
int main(int argc, char *argv[])
{
  FILE *input = nullptr;

  cout << "** Simple gdb for nuPython **" << endl;

  if (argc < 2)
  {
    //
    // missing filename:
    //
    cout << "usage: ./a.out filename.py" << endl;
    return 0;
  }

  //
  // assume 2nd arg is a nuPython file, try to open:
  //
  char *filename = argv[1];

  input = fopen(filename, "r");

  if (input == nullptr) // unable to open:
  {
    cout << "**ERROR: unable to open input file "
         << "'" << filename << "' for input." << endl;

    return 0;
  }

  //
  // parse the program, parser prints its own error message:
  //
  struct TokenQueue *tokens = parser_parse(input);

  if (tokens == nullptr)
  {
    fclose(input);
    return 0;
  }

  struct STMT *program = programgraph_build(tokens);
  struct RAM *memory = ram_init();
  struct STMT *cur = program;
  set<int> breakpoints;
  bool at_breakpoint = false;

  //
  // command loop:
  //
  string cmd;

  while (true)
  {

    cout << endl;
    cout << "Enter a command, h for help >" << endl;
    if (!(cin >> cmd)) // no more input, so quit
      break;

    if (cmd == "q")
    {
      //
      // quit:
      //
      break;
    }
    else if (cmd == "h")
    {
      help_command();
    }
    else if (cmd == "pg")
    {
      print_graph_command(program);
    }
    else if (cmd == "sm")
    {
      show_memory_command(memory);
    }
    else if (cmd == "b")
    {
      int n;
      cin >> n;
      breakpoint_command(breakpoints, n);
    }
    else if (cmd == "rb")
    {
      int n;
      cin >> n;
      remove_breakpoint_command(breakpoints, n);
    }
    else if (cmd == "cb")
    {
      clear_breakpoint_command(breakpoints);
    }
    else if (cmd == "lb")
    {
      list_breakpoint_command(breakpoints);
    }
    else if (cmd == "w")
    {
      where_command(cur);
    }
    else if (cmd == "s")
    {
      step_command(cur, memory, breakpoints, at_breakpoint);
    }
    else if (cmd == "r")
    {
      run_command(cur, memory, breakpoints, at_breakpoint);
    }
    else
    {
      cout << "unknown command" << endl;
    }

  } // while

  //
  // done: free data structures, close file, and return:
  //
  ram_destroy(memory);
  programgraph_destroy(program);
  tokenqueue_destroy(tokens);

  cout << endl;
  cout << "** Done **" << endl;

  fclose(input);

  return 0;
}
