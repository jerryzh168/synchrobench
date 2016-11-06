#include "rc_util.h"

void delete_SNode(SNode *);

void LFRCLoad(SNode **dest, SNode **A) {
  long r; SNode *a, *olddest = *dest;
  while (true) {
    a=*A;
    if(a==null){
      *dest = null; break;
    }
    r = a->rc;
    if(r>0 && DCAS(A,&a->rc, a, r, a, r+1)) {
      *dest = a;
      break;
    }
  }
  LFRCDestroy(olddest);
}

SNode* LFRCPass(Snode *v) {
  if (v!=null) add_to_rc(v,1);
  return v;
}

void LFRCDestroy(SNode *v) {
  if(v != null && add_to_rc(v, -1) == 1) {
      LFRCDestroy(v->L);
      LFRCDestroy(v->R);
  }
  delete_SNode(v);
}

long add_to_rc(SNode *v, int val) {
  long oldrc;
  while (true) {
    oldrc = v->rc;
    if (CAS(&v→rc, oldrc, oldrc+val))
      return oldrc;
  }
}

void LFRCStore(SNode **A, SNode *v) {
  SNode *oldval;
  if (v != null) add_to_rc(v, 1);
  while (true) {
    oldval = *A;
    if (CAS(A, oldval, v)) {
      LFRCDestroy(oldval);
      return;
    }
  }
}

void LFRCCopy(SNode **v, SNode *w) {
  Snode *oldv = *v;
  if (w != null) add_to_rc(w,1);
  *v=w;
  LFRCDestroy(oldv);
}

bool LFRCDCAS(SNode **A0, SNode **A1, SNode *old0, SNode *old1,
	      SNode *new0, SNode *new1) {
  if (new0 != null) add_to_rc(new0, 1);
  if (new1 != null) add_to_rc(new1, 1);
  if (DCAS(A0, A1, old0, old1, new0, new1)) {
    LFRCDestroy(old0,old1);
    return true;
  } else {
    LFRCDestroy(new0,new1);
    return false;
  }
}
