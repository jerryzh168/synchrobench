typedef valtype int;

class SNode {
  class SNode *L, *R;
  valtype V;
  long rc;
  SNode() : L(null), R(null), rc(1) {};
};

//void delete_SNode(SNode *);
void LFRCLoad(SNode **dest, SNode **A);
SNode* LFRCPass(Snode *v);
void LFRCDestroy(SNode *v);
long add_to_rc(SNode *v, int val);
void LFRCStore(SNode **A, SNode *v);
void LFRCCopy(SNode **v, SNode *w);
bool LFRCDCAS(SNode **A0, SNode **A1, SNode *old0, SNode *old1,
	      SNode *new0, SNode *new1);

