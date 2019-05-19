#include <iostream>
#include <bdd.h>
#include "bvec.h"

main(void){
  bdd_init(100,1000);
  bdd_setvarnum(16);

  bvec a = bvec_var(4,0, 2);
  bvec b = bvec_var(4,1, 2);
  bvec c = bvec_add(a,b);
  bdd_fnprintdot("bv_chk.dot", c[3]);
}
