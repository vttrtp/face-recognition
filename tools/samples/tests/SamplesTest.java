package idl.samples;

import java.util.List;

/**
 * Sample application that tests all generated Java bindings.
 * 
 * Build and run:
 *   # First build the native library
 *   cd tools/samples/build && cmake .. && make
 *   
 *   # Compile Java
 *   javac -d . java/src/main/java/idl/samples/*.java SamplesTest.java
 *   
 *   # Run with native library
 *   java -Djava.library.path=. idl.samples.SamplesTest
 */
public class SamplesTest {
    
    public static void main(String[] args) {
        System.out.println("=== IDL Samples Java Test ===\n");
        
        boolean allPassed = true;
        
        allPassed &= testCalculator();
        allPassed &= testGeometry();
        allPassed &= testShapeProcessor();
        allPassed &= testAsyncProcessor();
        
        System.out.println("\n=== Summary ===");
        if (allPassed) {
            System.out.println("All tests PASSED!");
        } else {
            System.out.println("Some tests FAILED!");
            System.exit(1);
        }
    }
    
    static boolean testCalculator() {
        System.out.println("Testing Calculator...");
        try (Calculator calc = new Calculator()) {
            boolean passed = true;
            
            passed &= assertEquals("add(2, 3)", 5, calc.add(2, 3));
            passed &= assertEquals("subtract(10, 4)", 6, calc.subtract(10, 4));
            passed &= assertEquals("multiply(3, 7)", 21, calc.multiply(3, 7));
            passed &= assertEquals("divide(10, 4)", 2.5, calc.divide(10.0, 4.0));
            passed &= assertEquals("getVersionMajor()", 1, calc.getVersionMajor());
            passed &= assertEquals("getVersionMinor()", 0, calc.getVersionMinor());
            
            System.out.println("  Calculator: " + (passed ? "PASSED" : "FAILED"));
            return passed;
        }
    }
    
    static boolean testGeometry() {
        System.out.println("Testing Geometry...");
        try (Geometry geom = new Geometry()) {
            boolean passed = true;
            
            List<Point> line = geom.createLine(0, 0, 100, 100, 5);
            passed &= assertEquals("createLine length", 5, line.size());
            passed &= assertEquals("createLine[0].x", 0, line.get(0).x);
            passed &= assertEquals("createLine[4].x", 100, line.get(4).x);
            
            List<BoundingBox> boxes = geom.findBoundingBoxes(3);
            passed &= assertEquals("findBoundingBoxes length", 3, boxes.size());
            passed &= assertEquals("boxes[0].x", 0, boxes.get(0).x);
            passed &= assertEquals("boxes[1].x", 10, boxes.get(1).x);
            
            passed &= assertEquals("getLastCount()", 3, geom.getLastCount());
            
            System.out.println("  Geometry: " + (passed ? "PASSED" : "FAILED"));
            return passed;
        }
    }
    
    static boolean testShapeProcessor() {
        System.out.println("Testing ShapeProcessor...");
        try (ShapeProcessor processor = new ShapeProcessor()) {
            boolean passed = true;
            
            BoundingBox box = new BoundingBox(10, 20, 100, 50, 0.9);
            passed &= assertEquals("calculateArea", 5000, processor.calculateArea(box));
            
            BoundingBox box2 = new BoundingBox(0, 0, 3, 4, 0.9);
            passed &= assertEquals("calculateDiagonal", 5.0, processor.calculateDiagonal(box2));
            
            Point p = new Point(10, 20);
            Point translated = processor.translate(p, 5, -3);
            passed &= assertEquals("translate.x", 15, translated.x);
            passed &= assertEquals("translate.y", 17, translated.y);
            
            BoundingBox created = processor.createBox(1, 2, 3, 4);
            passed &= assertEquals("createBox.x", 1, created.x);
            passed &= assertEquals("createBox.y", 2, created.y);
            passed &= assertEquals("createBox.width", 3, created.width);
            passed &= assertEquals("createBox.height", 4, created.height);
            
            BoundingBox testBox = new BoundingBox(0, 0, 100, 100, 1.0);
            Point inside = new Point(50, 50);
            Point outside = new Point(150, 150);
            passed &= assertEquals("boxContainsPoint(inside)", true, processor.boxContainsPoint(testBox, inside));
            passed &= assertEquals("boxContainsPoint(outside)", false, processor.boxContainsPoint(testBox, outside));
            
            System.out.println("  ShapeProcessor: " + (passed ? "PASSED" : "FAILED"));
            return passed;
        }
    }
    
    static boolean testAsyncProcessor() {
        System.out.println("Testing AsyncProcessor...");
        try (AsyncProcessor processor = new AsyncProcessor()) {
            boolean passed = true;
            
            // Test processWithProgress
            final int[] progressCount = {0};
            int result = processor.processWithProgress(5, (current, total) -> {
                progressCount[0]++;
                System.out.println("    Progress: " + current + "/" + total);
            });
            passed &= assertEquals("processWithProgress result", 5, result);
            passed &= assertEquals("progress callback count", 5, progressCount[0]);
            
            // Test countFiltered - count even numbers from 1 to 10
            int evenCount = processor.countFiltered(1, 10, value -> value % 2 == 0);
            passed &= assertEquals("countFiltered (even)", 5, evenCount);
            
            // Test sumTransformed - sum of squares from 1 to 5
            int sumSquares = processor.sumTransformed(1, 5, value -> value * value);
            passed &= assertEquals("sumTransformed (squares)", 55, sumSquares);
            
            System.out.println("  AsyncProcessor: " + (passed ? "PASSED" : "FAILED"));
            return passed;
        }
    }
    
    // Assertion helpers
    static boolean assertEquals(String name, int expected, int actual) {
        if (expected == actual) {
            return true;
        } else {
            System.out.println("    FAIL: " + name + " expected " + expected + " but got " + actual);
            return false;
        }
    }
    
    static boolean assertEquals(String name, double expected, double actual) {
        if (Math.abs(expected - actual) < 0.0001) {
            return true;
        } else {
            System.out.println("    FAIL: " + name + " expected " + expected + " but got " + actual);
            return false;
        }
    }
    
    static boolean assertEquals(String name, boolean expected, boolean actual) {
        if (expected == actual) {
            return true;
        } else {
            System.out.println("    FAIL: " + name + " expected " + expected + " but got " + actual);
            return false;
        }
    }
}
