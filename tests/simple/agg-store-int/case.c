
typedef struct
{
  char buf[10];
  int int_field;
} my_struct;

int main(int argc, char *argv[])
{
  my_struct s;

  s.int_field = 10;

  /*  BAD  */
  s.buf[s.int_field] = 'A';


  return 0;
}
