import java.lang.reflect.Field;

public class HumongousChain {
  private static final sun.misc.Unsafe _UNSAFE;

  static {
    try {
      Field unsafeField = sun.misc.Unsafe.class.getDeclaredField("theUnsafe");
      unsafeField.setAccessible(true);
      _UNSAFE = (sun.misc.Unsafe) unsafeField.get(null);
    } catch (Exception e) {
      throw new RuntimeException("SimplePartition: Failed to " + "get unsafe", e);
    }
  }

  public static void gc()
{
    System.out.println("=========================================");
    System.out.println("Call GC");
    System.gc();
    System.out.println("=========================================");
  }

  public static void access_all_objects(Object[] root) {
    System.out.println("Access Objects ----");
    
    Object[] tmp = root;
    Object last = tmp[tmp.length - 1];
    Integer logical = 0;

    while (last != null) {

      for (int i = 0; i < tmp.length - 1; i++) {
        logical |= (Integer) (tmp[i]);
      }

      tmp = (Object[]) last;
      last = tmp[tmp.length - 1];
    }

    System.out.println(logical);

    System.out.println("-------------------");
  }

  // Humongous --> |Humongous| --> Humongous
  //               ^^^^^^^^^^^
  //                marked H2
  public static void main(String[] args) {
    int total = 2000000; // ~ Should be around 30 MB

    // Humongous 0
    Object[] root = new Object[total];

    gc();

    // init objects except last
    for (int i = 0; i < total - 1; i++) {
      root[i] = new Integer(i);
    }
     
    // Humongous 1
    Object[] child = new Object[total];
    _UNSAFE.h2TagAndMoveRoot(child, 0, 0);

    // Add reference to another humongous
    root[total - 1] = child;

    // init objects except last
    for (int i = 0; i < total - 1; i++) {
      child[i] = new Integer(i);
    }
    
    // Humongous 2
    Object[] gchild = new Object[total];

    // init objects except last
    for (int i = 0; i < total - 1; i++) {
      gchild[i] = new Integer(i);
    }
    gchild[total - 1] = null;

    child[total - 1] = gchild;

    // Access Objects
    access_all_objects(root);
    // -------------

    gc();

    gchild = new Object[total];

    // init objects except last
    for (int i = 0; i < total - 1; i++) {
      gchild[i] = new Integer(i * 2);
    }
    gchild[total - 1] = null;

    child[total - 1] = gchild;

    // Access Objects
    access_all_objects(root);
    // -------------

    gc();

    // Access Objects
    access_all_objects(root);
    // -------------

    System.out.println(_UNSAFE.is_in_h2(root));
    System.out.println(_UNSAFE.is_in_h2(child));
    System.out.println(_UNSAFE.is_in_h2(gchild));
  }
}
