@0xbc49342ace2f15c3;

struct TreeNode {
  name @0 :Text;
  union {
    empty @1 :Void;
    array :group {
      data :group {
        slices @2 :List(Data);
        eos @3 :Bool;
      }
      type @4 :Type;
      len @5: UInt64;
      shape @6 :List(UInt64) = [];
    }
    children @7 :List(TreeNode);
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
