package face.detector;

import java.util.List;

/**
 * Demo application for face detection using the native library.
 */
public class FaceDetectorDemo {
    public static void main(String[] args) {
        if (args.length < 2) {
            System.out.println("Usage: FaceDetectorDemo <cascade_path> <image_path>");
            System.exit(1);
        }

        String cascadePath = args[0];
        String imagePath = args[1];

        System.out.println("Face Detection Demo");
        System.out.println("==================");
        System.out.println("Cascade: " + cascadePath);
        System.out.println("Image: " + imagePath);
        System.out.println();

        try (FaceDetector detector = new FaceDetector(cascadePath)) {
            if (!detector.isLoaded()) {
                System.err.println("Failed to load cascade file!");
                System.exit(1);
            }

            System.out.println("Detector loaded successfully!");
            
            List<FaceRect> faces = detector.detectFromFile(imagePath);
            
            System.out.println("Detected " + faces.size() + " face(s):");
            for (int i = 0; i < faces.size(); i++) {
                FaceRect face = faces.get(i);
                System.out.printf("  Face %d: x=%d, y=%d, width=%d, height=%d%n",
                    i + 1, face.x, face.y, face.width, face.height);
            }
        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }
    }
}
