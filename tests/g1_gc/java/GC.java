import jdk.test.whitebox.WhiteBox;
import jdk.test.lib.Helpers;

import java.lang.reflect.Field;

public class GC{
  static WhiteBox wb = WhiteBox.getWhiteBox();

  public static void gc() {
    System.out.println("=========================================");
    System.out.println("Call GC");

    Helpers.waitTillCMCFinished(wb, 10);
    wb.g1StartConcMarkCycle();
    Helpers.waitTillCMCFinished(wb, 10);

    wb.youngGC(); // the "Prepare Mixed" gc --> only young objs
    wb.youngGC(); // the "Mixed" gc
    wb.youngGC(); // the "Mixed" gc		
    wb.youngGC(); // the "Mixed" gc

    System.out.println("=========================================");
  }

  public static void full_gc() {
    System.out.println("=========================================");
    System.out.println("Full GC");

    System.gc();

    System.out.println("=========================================");
  }

  public static void cm_start() {
    Helpers.waitTillCMCFinished(wb, 10);

    System.out.println("=========================================");
    System.out.println("Concurrent Marking");

    wb.g1StartConcMarkCycle();

    System.out.println("=========================================");
  }

  public static void wait_cm() {
    Helpers.waitTillCMCFinished(wb, 10);

    System.out.println("== CM finished");
  }

  public static void young_gc() {
    wb.youngGC();
  }

  // -XX:+AlwaysTenure should be enabled
  public static void move_to_old(){
    wb.youngGC();
  }
}
