/*pgutil.cpp*/

/**
  * @brief program graph utility functions
  *
  * @note SOLUTION
  *
  * @author original template by Prof. Joe Hummel
  * @author ?
  * @author ?
  * @author ?  
  * @note Northwestern University
  */

#include <iostream>

using std::cout;
using std::endl;

#include "pgutil.h"


/**
 * @brief breaks the program graph at the given stmt
 *
 * Breaks the links between the cur statement and the one 
 * that follows; in the case of while loops and if stmts,
 * there are multiple links that must be broken. Returns 
 * a struct that contains copies of all the broken links.
 *
 * NOTE: not all the links in the struct will be valid. For
 * example, if cur denotes a while loop, then only struct.next
 * and struct.loop_body will be valid --- ignore the other
 * links.
 *
 * @param cur denotes stmt where to break program graph
 * @return struct PG_LINKS that contains saved copies of broken links
 */
struct PG_LINKS break_pg(struct STMT* cur)
{
  struct PG_LINKS links;

  links.next = nullptr;
  links.loop_body = nullptr;
  links.true_path = nullptr;
  links.false_path = nullptr;

  if (cur == nullptr) return links;

  switch (cur->stmt_type) {
    case STMT_ASSIGNMENT:
      links.next = cur->types.assignment->next_stmt;
      cur->types.assignment->next_stmt = nullptr;
      break;

    case STMT_FUNCTION_CALL:
      links.next = cur->types.function_call->next_stmt;
      cur->types.function_call->next_stmt = nullptr;
      break;

    case STMT_PASS:
      links.next = cur->types.pass->next_stmt;
      cur->types.pass->next_stmt = nullptr;
      break;

    case STMT_WHILE_LOOP:
      links.loop_body = cur->types.while_loop->loop_body;
      links.next = cur->types.while_loop->next_stmt;
      cur->types.while_loop->loop_body = nullptr;
      cur->types.while_loop->next_stmt = nullptr;
      break;

    case STMT_IF_THEN_ELSE:
      links.true_path = cur->types.if_then_else->true_path;
      links.false_path = cur->types.if_then_else->false_path;
      cur->types.if_then_else->true_path = nullptr;
      cur->types.if_then_else->false_path = nullptr;
      break;
  }

  // maybe do some default in case we have an error
  return links;
}

/**
 * @brief repairs the program graph at the given stmt
 *
 * This function is the counterpart to break_pg by repairing
 * the program graph using the saved links in the given 
 * struct. This function only works if it is called to repair
 * the program graph at the SAME STATEMENT where it was 
 * broken. In other words, cur in this case must denote the 
 * SAME STMT cur in the call to break_pg.
 *
 * NOTE: not all the links in the struct are valid. For
 * example, if cur denotes a while loop, then only struct.next
 * and struct.loop_body will be valid --- ignore the other
 * links.
 *
 * @param cur denotes stmt where to repair program graph
 * @param struct PG_LINKS containing saved copies of broken links
 */
void repair_pg(struct STMT* cur, struct PG_LINKS links)
{
  if (cur == nullptr) return;

  switch (cur->stmt_type) {
    case STMT_ASSIGNMENT:
      cur->types.assignment->next_stmt = links.next;
      break;

    case STMT_FUNCTION_CALL:
      cur->types.function_call->next_stmt = links.next;
      break;

    case STMT_PASS:
      cur->types.pass->next_stmt = links.next;
      break;

    case STMT_WHILE_LOOP:
      cur->types.while_loop->loop_body = links.loop_body;
      cur->types.while_loop->next_stmt = links.next;
      break;

    case STMT_IF_THEN_ELSE:
      cur->types.if_then_else->true_path = links.true_path;
      cur->types.if_then_else->false_path = links.false_path;
      break;
  }
}


#ifdef UNIT_TEST
//////////////////////////////////////////////////////////////////////////
// 
// UNIT TESTS for above functions:
//
//////////////////////////////////////////////////////////////////////////

#include "gtest/gtest.h"

//
// Since we are working in C++, we have to tell the
// C++ compiler that the functions in the provided
// nuPython code are C functions:
//
extern "C" {
  #include "tokenqueue.h"
  #include "parser.h"
  #include "programgraph.h"
  #include "ram.h"
  #include "execute.h"
}

TEST(break_repair_pg, assignment)
{
  cout << "**starting" << endl;

  FILE* file;
  char* filename = "__test.py";

  file = fopen(filename, "w");
  fprintf(file, "x = 0\n");
  fprintf(file, "y = 1\n");
  fclose(file);

  file = fopen(filename, "r");
  struct TokenQueue* tokens = parser_parse(file);
  struct STMT* program = programgraph_build(tokens);
  fclose(file);

  struct STMT* stmt2 = program->types.assignment->next_stmt;

  //
  // okay let's break PG at the first stmt:
  //
  cout << "**breaking..." << endl;

  struct PG_LINKS links = break_pg(program);

  ASSERT_EQ(program->types.assignment->next_stmt, nullptr);
  ASSERT_EQ(links.next, stmt2);

  //
  // now let's repair it:
  //
  cout << "**repairing..." << endl;

  repair_pg(program, links);

  ASSERT_EQ(program->types.assignment->next_stmt, stmt2);

  //
  // done!
  //
  tokenqueue_destroy(tokens);
  programgraph_destroy(program);

  cout << "**passed!" << endl;
}  

TEST(break_repair_pg, pass)
{
  cout << "**starting" << endl;

  FILE* file;
  char* filename = "__test.py";

  file = fopen(filename, "w");
  fprintf(file, "pass\n");
  fprintf(file, "y = 1\n");
  fclose(file);

  file = fopen(filename, "r");
  struct TokenQueue* tokens = parser_parse(file);
  struct STMT* program = programgraph_build(tokens);
  fclose(file);

  struct STMT* stmt2 = program->types.pass->next_stmt;

  //
  // okay let's break PG at the first stmt:
  //
  cout << "**breaking..." << endl;

  struct PG_LINKS links = break_pg(program);

  ASSERT_EQ(program->types.pass->next_stmt, nullptr);
  ASSERT_EQ(links.next, stmt2);

  //
  // now let's repair it:
  //
  cout << "**repairing..." << endl;

  repair_pg(program, links);

  ASSERT_EQ(program->types.pass->next_stmt, stmt2);

  //
  // done!
  //
  tokenqueue_destroy(tokens);
  programgraph_destroy(program);

  cout << "**passed!" << endl;
}  

//
// helper: writes the given source to __test.py and builds the graph
//
static struct STMT* build_from(const char* src, struct TokenQueue*& tokens)
{
  FILE* file = fopen("__test.py", "w");
  fprintf(file, "%s", src);
  fclose(file);

  file = fopen("__test.py", "r");
  tokens = parser_parse(file);
  fclose(file);

  return programgraph_build(tokens);
}

TEST(break_repair_pg, function_call)
{
  struct TokenQueue* tokens;
  struct STMT* program = build_from("print('hi')\ny = 1\n", tokens);

  ASSERT_EQ(program->stmt_type, STMT_FUNCTION_CALL);
  struct STMT* stmt2 = program->types.function_call->next_stmt;
  ASSERT_NE(stmt2, nullptr);

  struct PG_LINKS links = break_pg(program);
  ASSERT_EQ(program->types.function_call->next_stmt, nullptr);
  ASSERT_EQ(links.next, stmt2);

  repair_pg(program, links);
  ASSERT_EQ(program->types.function_call->next_stmt, stmt2);

  tokenqueue_destroy(tokens);
  programgraph_destroy(program);
}

TEST(break_repair_pg, while_loop)
{
  struct TokenQueue* tokens;
  struct STMT* program = build_from(
    "x = 3\n"
    "while x > 0:\n"
    "{\n"
    "  x = x - 1\n"
    "}\n"
    "print(x)\n", tokens);

  struct STMT* loop = program->types.assignment->next_stmt;
  ASSERT_EQ(loop->stmt_type, STMT_WHILE_LOOP);

  struct STMT* body  = loop->types.while_loop->loop_body;
  struct STMT* after = loop->types.while_loop->next_stmt;
  ASSERT_NE(body, nullptr);
  ASSERT_NE(after, nullptr);

  struct PG_LINKS links = break_pg(loop);
  ASSERT_EQ(loop->types.while_loop->loop_body, nullptr);
  ASSERT_EQ(loop->types.while_loop->next_stmt, nullptr);
  ASSERT_EQ(links.loop_body, body);
  ASSERT_EQ(links.next, after);

  repair_pg(loop, links);
  ASSERT_EQ(loop->types.while_loop->loop_body, body);
  ASSERT_EQ(loop->types.while_loop->next_stmt, after);

  tokenqueue_destroy(tokens);
  programgraph_destroy(program);
}

TEST(break_repair_pg, if_else)
{
  struct TokenQueue* tokens;
  struct STMT* program = build_from(
    "x = 3\n"
    "if x > 0:\n"
    "{\n"
    "  y = 1\n"
    "}\n"
    "else:\n"
    "{\n"
    "  y = 2\n"
    "}\n"
    "print(y)\n", tokens);

  struct STMT* ifs = program->types.assignment->next_stmt;
  ASSERT_EQ(ifs->stmt_type, STMT_IF_THEN_ELSE);

  struct STMT* t    = ifs->types.if_then_else->true_path;
  struct STMT* f    = ifs->types.if_then_else->false_path;
  struct STMT* next = ifs->types.if_then_else->next_stmt;
  ASSERT_NE(t, nullptr);
  ASSERT_NE(f, nullptr);

  struct PG_LINKS links = break_pg(ifs);
  ASSERT_EQ(ifs->types.if_then_else->true_path, nullptr);
  ASSERT_EQ(ifs->types.if_then_else->false_path, nullptr);
  ASSERT_EQ(ifs->types.if_then_else->next_stmt, next);  // left alone
  ASSERT_EQ(links.true_path, t);
  ASSERT_EQ(links.false_path, f);

  repair_pg(ifs, links);
  ASSERT_EQ(ifs->types.if_then_else->true_path, t);
  ASSERT_EQ(ifs->types.if_then_else->false_path, f);

  tokenqueue_destroy(tokens);
  programgraph_destroy(program);
}

TEST(break_repair_pg, last_stmt)
{
  struct TokenQueue* tokens;
  struct STMT* program = build_from("x = 1\n", tokens);

  ASSERT_EQ(program->types.assignment->next_stmt, nullptr);

  struct PG_LINKS links = break_pg(program);
  ASSERT_EQ(links.next, nullptr);

  repair_pg(program, links);
  ASSERT_EQ(program->types.assignment->next_stmt, nullptr);

  tokenqueue_destroy(tokens);
  programgraph_destroy(program);
}

TEST(break_repair_pg, step_executes_one_stmt)
{
  struct TokenQueue* tokens;
  struct STMT* program = build_from("x = 1\nx = 2\n", tokens);
  struct RAM* memory = ram_init();

  struct PG_LINKS links = break_pg(program);
  execute(program, memory);
  repair_pg(program, links);

  struct RAM_VALUE* v = ram_read_cell_by_name(memory, "x");
  ASSERT_NE(v, nullptr);
  ASSERT_EQ(v->types.i, 1);   // second stmt must NOT have run

  ram_free_value(v);
  ram_destroy(memory);
  tokenqueue_destroy(tokens);
  programgraph_destroy(program);
}


//////////////////////////////////////////////////////////////////////////
// 
// END OF UNIT TESTS
//
//////////////////////////////////////////////////////////////////////////
#endif
