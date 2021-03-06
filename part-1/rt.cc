// Yamato Eguchi
// CPSC 120-01
// 2021-05-06
// yamatoe1227@csu.fullerton.edu
// @yamatoeguchi
//
// Lab 12-01
//
//This is my many spheres
//

#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>

#include "image.h"
#include "ray.h"
#include "sphere.h"
#include "utility.h"
#include "vec3.h"

using namespace std;

/// RayColor computes the color the ray \p r sees through the camera lens.
/// Given a Ray \p r, calculate the color that is visible to the ray.
/// \param r A ray eminating from the camera through the scene
/// \param world The world - the scene that will be rendered
/// \returns The color visible from that ray
Color RayColor(const Ray& r, const std::vector<std::shared_ptr<Hittable>>& world) {
  HitRecord rec;
  Color c;
  HitRecord tmp_rec;
  bool hit_anything = false;
  double t_min = 0.0;
  double closest_so_far = kInfinity;
  for (const auto& object : world) {
    if (object->hit(r, t_min, closest_so_far, tmp_rec)) {
      hit_anything = true;
      closest_so_far = tmp_rec.t;
      rec = tmp_rec;
    }
  }
  // check to see if the ray intersects with the world
  if (hit_anything) {
    c = rec.material->reflect_color(r, rec);
    // This is the base color of the sphere. You may set this color
    // to whatever you like.
  } else {
    // Color sky_top{.5, .4, 0};
    // Color sky_bottom{.2, .1, .5};
    Color sky_top{0.4980392156862745, 0.7450980392156863, 0.9215686274509803};
    Color sky_bottom{1, 1, 1};
    Vec3 unit_direction = UnitVector(r.direction());
    double t = 0.5 * (unit_direction.y() + 1.0);
    c = (1.0 - t) * sky_bottom + t * sky_top;
  }
  return c;
}

/// ErrorMessage prints out \p message first and then prints the standard
/// message
/// \code
/// "There was an error. Exiting.\n".
/// \endcode
///
/// \param message The programmer defined string that specifies the current
/// error.
void ErrorMessage(const string& message) {
  cout << message << "\n";
  cout << "There was an error. Exiting.\n";
}

/// Main function - Entry point for our ray tracer
/// The program takes one argument which is the output file name. The image
/// generated by the main function is written out as a PPM file. Use the
/// Graphics Magick package to convert the output PPM image to a PNG or JPEG
/// image file.
/// \remark To install Graphics Magick on an Ubuntu system `sudo apt install
/// graphicsmagick`.
/// \remark To convert from a PPM to a PNG, `gm convert input.ppm output.png`
/// \remark To convert from a PPM to a JPG, `gm convert input.ppm output.png`
int main(int argc, char const* argv[]) {
  if (argc < 2) {
    ErrorMessage("Please provide a path to a file.");
    exit(1);
  }
  string argv_one_output_file_name = string(argv[1]);

  /// Image definition in main
  /// The image is the output from the virtual camera. The image is what
  /// you will see when you open the output in an image viewer. The image's
  /// dimensions are specified in pixels and are therefore in integer units.
  /// The aspect ratio represents the ratio of width / height. The ratio
  /// 16:9 is the ratio used for wide format movies. Traditional 35mm film
  /// photographs have an image that is 36 mm x 24 mm which has an aspect
  /// ratio of 36:24 or 1.5.
  const double kAspectRatio = 16.0 / 9.0;
  // Set the image width to 400 pixels
  const int kImageWidth = 1000;
  // Calculate the height of the image using the width and aspect ratio.
  // Remember to round the number to the closest integer.
  const int kImageHeight = int(lround(kImageWidth / kAspectRatio));
  // Create a new Image object using the file name provided on the
  // command line.
  const int kSamplesPerPixel = 20;

  Image image(argv_one_output_file_name, kImageWidth, kImageHeight);
  if (!image.is_open()) {
    ostringstream message_buffer("Could not open the file ", ios_base::ate);
    // String streams can use the same operators as cout and cin.
    message_buffer << argv_one_output_file_name << "!";
    // Convert the string stream to a string before passing it to
    // ErrorMessage().
    ErrorMessage(message_buffer.str());
    exit(1);
  }
  cout << "Image: " << image.height() << "x" << image.width() << "\n";

  /// World definition in main
  // auto world = OriginalScene();
  const int kNumberSpheres = 1000;
  auto world = RandomScene(kNumberSpheres);

  /// Camera definition in main
  /// The [viewport](https://en.wikipedia.org/wiki/Viewport) is the
  /// rectangular viewing region visible to the camera. You can think of it
  /// as the dimensions of the camera's film or sensor. The coordinates
  /// are specfied in floating point units (doubles). Unlike an image which
  /// has its dimensions expressed as pixels, the camera's viewport dimensions
  /// can be expressed in whatever units you would like to use. Since we are
  /// creating a make believe image of make believe things, you can imagine
  /// the size of the objects to be as small (millimeters) or as large
  /// (kilometers) as you like.
  // The height of the viewport
  const double kViewportHeight = 2.0;
  // The width of the viewport is calculated using the height and the
  // previously defined aspect ratio.
  const double kViewportWidth = kAspectRatio * kViewportHeight;
  // The focal length is the distance between the projection plane (the end
  // of the lens) and the projection point (the inside of the camera).
  const double kFocalLength = 1.0;
  // The origin is as we expect. Everything in our world will be measured
  // from the center of the camera.
  const Point3 kOrigin{0, 0, 0};
  // Create a vector that represents the horizontal direction with respect
  // to the viewport.
  const Vec3 kHorizontal{kViewportWidth, 0, 0};
  // Create a vecotr that represents the vertical direction with respect
  // to the viewport
  const Vec3 kVertical{0, kViewportHeight, 0};
  // Calculate a vector that starts at the origin and points to the lower
  // left hand corner of the viewport. We will use this when we generate
  // all the rays that emanate out from the viewplane.
  const Vec3 kLowerLeftCorner =
      kOrigin - kHorizontal / 2 - kVertical / 2 - Vec3(0, 0, kFocalLength);

  /// Rendering the image in main
  /// Using nested loops, start from the top row of the viewplane and
  /// calculate the color for each pixel in each column of the image.
  /// The image (the output file) is the result of this calculation so
  /// keep in mind that each ray that is created maps onto a pixel in the
  /// image.
  // Save the starting time so the elapsed time can
  // be calculated.
  chrono::time_point<chrono::high_resolution_clock> start =
      chrono::high_resolution_clock::now();
  Color pixel_color;
  for (int row = image.height() - 1; row >= 0; row--) {
    for (int column = 0; column < image.width(); column++) {
      for (int s = 0; s < kSamplesPerPixel; s++) {
        // u is the distance from the left edge of the image to the right
        // edge of the image measured as a percentage.
        // The column divided by the image.width() - 1 is the percentage of
        // the distance from the left side of the image to the right.
        // Consider column = 0 then colum / (image.width() - 1) -> 0.0
        //          column = (image.width() - 1) / 2 then
        //          colum / (image.width() - 1) -> 0.5
        //          column = image.width() - 1 then
        //          colum / (image.width() - 1) -> 1.0
        // The same is true for v
        double u = (double(column) + RandomDouble01())/ double(image.width() - 1);
        double v = (double(row) + RandomDouble01()) / double(image.height() - 1);
        // Create a ray that starts at the camera's center, the origin, and
        // travels through the pixel center defined by
        // kLowerLeftCorner + u * kHorizontal + v * kVertical - kOrigin
        Ray r{kOrigin, 
              kLowerLeftCorner + u * kHorizontal + v * kVertical - kOrigin};
        pixel_color = pixel_color + RayColor(r, world);
      }
        // cout << "row: " << row << " col: " << column << " u: " << u << " v: "
        // << v << "\n" << r << "\n";
        // Calculate and return the color at the pixel that Ray r
        // points through.
        // Write the color to the image file.
      double scale = 1.0 / double(kSamplesPerPixel);
      double r = Clamp(std::sqrt(pixel_color.r() * scale), 0.0, 1.0);
      double g = Clamp(std::sqrt(pixel_color.g() * scale), 0.0, 1.0);
      double b = Clamp(std::sqrt(pixel_color.b() * scale), 0.0, 1.0);
      pixel_color = Color(r, g, b);
      image.write(pixel_color);

    }
  }
  // Save the ending time
  chrono::time_point<chrono::high_resolution_clock> end =
      chrono::high_resolution_clock::now();

  // Calculate the elapsed time by taking the difference between end
  // and start.
  chrono::duration<double> elapsed_seconds = end - start;
  // Display the elapsed number of seconds.
  cout << "Time elapsed: " << elapsed_seconds.count() << " seconds.\n";

  return 0;
}
