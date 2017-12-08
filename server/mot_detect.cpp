

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <iostream>
#include <tuple>
#include <camerasp/smtpClient.hpp>
using namespace std;
using namespace cv;

// Check if there is motion in the result matrix
// count the number of changes and return.
std::tuple<cv::Rect,int> detect_motion(const Mat& motion, const Rect& bounding_box )
{
  int number_of_changes = 0;
  int min_x = motion.cols, max_x = 0;
  int min_y = motion.rows, max_y = 0;
  // loop over image and detect changes
  for(int j = 0 ; j < bounding_box.height; j+=2){ // height
    for(int i = 0 ; i < bounding_box.width; i+=2){ // height
      // check if at pixel (j,i) intensity is equal to 255
      // this means that the pixel is different in the sequence
      // of images (prev_frame, current_frame, next_frame)
      if(static_cast<int>(motion.at<int>(bounding_box.y+j,bounding_box.x+i)) == 255)
      {
        number_of_changes++;
        if(min_x>i) min_x = bounding_box.x+i;
        if(max_x<i) max_x = bounding_box.x+i;
        if(min_y>j) min_y = bounding_box.y+j;
        if(max_y<j) max_y = bounding_box.y+j;
      }
    }
  }
  if(number_of_changes){
    // draw rectangle round the changed pixel
    Point x(min_x,min_y);
    Point y(max_x,max_y);
    Rect rect(x,y);
    return make_tuple(rect,number_of_changes);
  }
  return make_tuple(cv::Rect(),0);
}
// When motion is detected we write the image to disk
//    - Check if the directory exists where the image will be stored.
//    - Build the directory and image names.
void handle_motion(const char* fName, smtp_client& smtp)
{
  static int current_state=0;

  CURLcode res = CURLE_OK;
  static  Mat current_frame ;
  static  Mat next_frame ;
  // d1 and d2 for calculating the differences
  // result, the result of and operation, calculated on d1 and d2
  // number_of_changes, the amount of changes in the result matrix.
  // color, the color for drawing the rectangle when something has changed.
  int number_of_changes;
  static int number_of_sequence = 0;

  // Detect motion in window
  int x_start = 10;
  int y_start = 10;
  switch ( current_state)
  {
    case 0:
      current_frame = cv::imread(fName, CV_LOAD_IMAGE_COLOR) ;
      cvtColor(current_frame, current_frame, CV_RGB2GRAY);
      current_state =1;
      return;
    case 1:
      next_frame =  cv::imread(fName, CV_LOAD_IMAGE_COLOR) ;
      cvtColor(next_frame, next_frame, CV_RGB2GRAY);
      current_state=2;
      return;
    case 2:
      {


        // If more than 'there_is_motion' pixels are changed, we say there is motion
        // and store an image on disk
        int there_is_motion = 250;


        // Erode kernel
        Mat kernel_ero = getStructuringElement(MORPH_RECT, Size(2,2));

        // Take a new image
        Mat  prev_frame;
        cv::swap(prev_frame,current_frame);
        cv::swap(current_frame , next_frame);
        next_frame =  cv::imread(fName, CV_LOAD_IMAGE_COLOR) ;
        if(! next_frame .data )                              // Check for invalid input
        {
          return;
        }
        cvtColor(next_frame, next_frame, CV_RGB2GRAY);

        // Calc differences between the images and do AND-operation
        // threshold image, low differences are ignored (ex. contrast change due to sunlight)
        Mat d1, d2, motion;
        absdiff(prev_frame, next_frame, d1);
        absdiff(next_frame, current_frame, d2);
        bitwise_and(d1, d2, motion);
        threshold(motion, motion, 35, 255, CV_THRESH_BINARY);
        erode(motion, motion, kernel_ero);
        Rect rect;
        int width = current_frame.cols-20;
        int height = current_frame.rows-20;
        std::tie(rect,number_of_changes) = detect_motion(motion,  cv::Rect( x_start,  y_start, width,height));
        std::cout << number_of_changes << std::endl;
        // If a lot of changes happened, we assume something changed.
        if(number_of_changes>=there_is_motion)
        {
          smtp.file_name = fName;
          //if(number_of_sequence>0)
          { 
            std::cout << "Image Name: " << smtp.file_name  << "\n"
              << "Top Left: " << rect.x << " " << rect.y << "\n" 
              << "Dimensions: " << rect.width << " " << rect.height << "\n" ;
            smtp.send();
          }
          number_of_sequence++;
        }
        else
          number_of_sequence = 0;
      }
      break ;    
  }
  return;
}
