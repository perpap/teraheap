import java.lang.reflect.Field;
import java.util.ArrayList;

// ---------------------------------------------------------------------------
// NOTE:
// To run this test, you need to pass -XX:+ExplicitGCInvokesConcurrent
// You need to check the GC log that a full gc is triggered during CM
// ---------------------------------------------------------------------------
public class FGCAfterCM {
  private static final sun.misc.Unsafe _UNSAFE;
  private static final int SIZE = 10000;

	static {
		try {
			Field unsafeField = sun.misc.Unsafe.class.getDeclaredField("theUnsafe");
			unsafeField.setAccessible(true);
			_UNSAFE = (sun.misc.Unsafe) unsafeField.get(null);
		} catch (Exception e) {
			throw new RuntimeException("SimplePartition: Failed to " + "get unsafe", e);
		}
	}

  static void pressure() {
    // Create an array of objects of class A
    A[] arrayOfA = new A[SIZE * 3];

    // Instantiate objects of class A and assign them to the array
    for (int i = 0; i < SIZE * 3; i++) {
      arrayOfA[i] = new A(i);
    }
  }

  public static void main(String[] args) {
    int iters = 100;
    // Create the array list that will hold all the arrays
    ArrayList<A[]> arraysOfA = new ArrayList<A[]>();

    for (int i = 0; i < iters; i++) {
      // Create an array of objects of class A
      A[] arrayOfA = new A[SIZE];

      // Instantiate objects of class A and assign them to the array
      for (int j = 0; j < SIZE; j++) {
        arrayOfA[j] = new A(j);
      }

      // Mark to move in H2
      _UNSAFE.h2TagAndMoveRoot(arrayOfA, 0, 0);

      arraysOfA.add(arrayOfA);

      // Every x iterations trigger a concurrent marking and try to
      // put pressure on the heap
      int x = 5;

      if ((i % x) == 0) {
        // Trigger CM (remeber to enable the flag)
        System.gc();
        pressure(); // This should trigger a full GC
      }
    }

    // Access all objects
    for (int i = 0; i < iters; i++) {
      int sum = 0;
      A[] array = arraysOfA.get(i);

      for (int j = 0; j < SIZE; j++) {
        sum += array[j].x;
      }

      System.out.println(sum);
    }
  }
}

