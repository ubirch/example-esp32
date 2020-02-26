#include "unity.h"//something???
#include "math_d.h"//  "math_d.h"


TEST_CASE("RIGHT ADDTITION", "[decke]"){
    int a = 3;
    int b = 56;

    int c = plus(a,b);
    TEST_ASSERT(c == (a+b));

}

TEST_CASE("float vs int ADDTITION", "[decke]"){
    int a = 3;
    float b=56.7;

    int c = plus(a,b);
    TEST_ASSERT(c == (a+b));

}
TEST_CASE("no ! RIGHT ADDTITION", "[fails]"){
    int a = 3;
    int b = 56;

    int c = plus(a,b);
    TEST_ASSERT(c != (a+b));

}



TEST_CASE("STRING justincase COUCOU", "[decke]"){
  //    char chaine[] = "Salut";
    char  a[] = "etwas\0"; // \0 before the end schreiben zu sehen ob es crasht
    char  b[] = "daniel ääääÖÖö§34²";

    char * c = string_together(a,b);
    printf("%s\r\n", c);

//   TEST_ASSERT(c == string_together(a,b));// LoadProhibited
 //   TEST_ASSERT(c != string_together(a,b)); //StoreProhibited
   // TEST_ASSERT(c == strcat(a,b));

}
