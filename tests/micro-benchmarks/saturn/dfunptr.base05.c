// type-based throttling of funptr propagation

void fnptr0(int x) {}
void fnptr1(int x) {}
void fnptr2(int x) {}
void fnptr3(int x) {}

void* ret_funptr0() { return fnptr0; }
long ret_funptr1() { return (long)fnptr1; }

void consume_ret()
{
  void (*fn0)(int) = ret_funptr0();
  void (*fn1)(int) = ret_funptr1();
  fn0(0);
  fn1(1);
}

void consume_arg(void *v, long l)
{
  void (*fn0)(int) = v;
  void (*fn1)(int) = l;
  fn0(0);
  fn1(1);
}

void call_arg()
{
  consume_arg(fnptr2,fnptr3);
}

// test annotations

# 1 "drivers/block/paride/pd.c"

struct request { void *special; };
static struct request *pd_req;

struct pd_unit *pd_current;

static int pd_special(void)
{
  int (*func)(struct pd_unit*) = pd_req->special;
  return func(pd_current);
}

static void pd_special_command(struct pd_unit *disk,
                               int (*func)(struct pd_unit *disk))
{
  struct request rq;
  rq.special = func;
}

int pdfn0(struct pd_unit*) { return 0; }
int pdfn1(struct pd_unit*) { return 1; }

void call_special(struct pd_unit *disk)
{
  pd_special_command(disk,pdfn0);
  pd_special_command(disk,pdfn1);
}
