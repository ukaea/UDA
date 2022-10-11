@0xbc49342ace2f15c3;

struct TreeNode {
  name @0 :Text;
  union {
    empty @1 :Void;
    array: group {
      data @2 :Data;
      type @3 :Type;
      len @4: UInt64;
      shape @5 :List(UInt64) = [];
    }
    children @6 :List(TreeNode);
  }

  enum Type {
    int8 @0;
    int16 @1;
    int32 @2;
    int64 @3;
    uint8 @4;
    uint16 @5;
    uint32 @6;
    uint64 @7;
    flt32 @8;
    flt64 @9;
    string @10;
    void @11;
  }
}
