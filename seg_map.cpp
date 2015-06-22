///////////////////////////////////////////////////////////
////// seg_map.cpp
////// 2015-06-22
////// Tairui Chen
///////////////////////////////////////////////////////////

#include <cuda_runtime.h>

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <vector>

#include <opencv/highgui.h>
#include <opencv/cv.h> //cvResize()

#include "caffe/caffe.hpp"

#define IMAGE_SIZE 224

using namespace caffe;  // NOLINT(build/namespaces)
using namespace std;
using namespace cv;

void bubble_sort(float *feature, int *sorted_idx)
{
	int i=0, j=0;
	float tmp;
	int tmp_idx;

	for(i=0; i<1000; i++)
		sorted_idx[i] = i;

	for(i=0; i<1000; i++)
	{
		for(j=0; j<999; j++)
		{
			if(feature[j] < feature[j+1])
			{
				tmp = feature[j];
				feature[j] = feature[j+1];
				feature[j+1] = tmp;

				tmp_idx = sorted_idx[j];
				sorted_idx[j] = sorted_idx[j+1];
				sorted_idx[j+1] = tmp_idx;
			}
		}
	}
}

void get_top5(float *feature, int arr[5])
{
	int i=0;
	int sorted_idx[1000];
	
	bubble_sort(feature, sorted_idx);

	for(i=0; i<5; i++)
	{
		arr[i] = sorted_idx[i];
	}
}

void get_label(char filename[256], char label[][512])
{
	FILE *fp = fopen(filename, "r");

	int i=0, j=0;


	for(i=0; i<1000; i++)
	{
		fgets(label[i], 512, fp);

		for(j=0; j<512; j++)
		{
			if(label[i][j] == '\n' || label[i][j] == ',')
			{
				label[i][j] = '\0';
				break;
			}
		}
	}

	fclose(fp);
}

void draw_output(IplImage *img, float *output, int *idx, char label[][512])
{
	int i=0;
	CvFont font;
	char str[256];
	cvInitFont(&font, CV_FONT_HERSHEY_PLAIN,1.5,1.5,0,2,8);

	for(i=0; i<5; i++)
	{
		sprintf(str, "%s, %.03f", label[idx[i]], output[i]);
		if(i == 0)
			cvPutText(img, str, cvPoint(10, 30+i*30),&font, CV_RGB(255,0,0));
		else if(i == 1)
			cvPutText(img, str, cvPoint(10, 30+i*30),&font, CV_RGB(0,0,255));
		else
			cvPutText(img, str, cvPoint(10, 30+i*30),&font, CV_RGB(255,255,0));
	}
}

int main()
{
	// Test mode
	Caffe::set_phase(Caffe::TEST);

	// mode setting - CPU/GPU
	Caffe::set_mode(Caffe::GPU);

	// gpu device number
	int device_id = 0;
	Caffe::SetDevice(device_id);

	// prototxt
	//Net<float> caffe_test_net("C:/Users/cht2pal/Desktop/caffe-old-unpool/examples/live_net/VGG_ILSVRC_19_layers_deploy.prototxt");
	Net<float> caffe_test_net("C:/Users/cht2pal/Desktop/caffe-old-unpool/examples/DeconvNet/model/DeconvNet/DeconvNet_inference_deploy.prototxt");
	// caffemodel
	//caffe_test_net.CopyTrainedLayersFrom("C:/Users/cht2pal/Desktop/caffe-old-unpool/examples/live_net/VGG_ILSVRC_19_layers.caffemodel");
	caffe_test_net.CopyTrainedLayersFrom("C:/Users/cht2pal/Desktop/caffe-old-unpool/examples/DeconvNet/model/DeconvNet/DeconvNet_trainval_inference.caffemodel");

	//Debug
	//caffe_test_net.set_debug_info(true);

	// read labels
	char label[1000][512];
	get_label("C:/Users/cht2pal/Desktop/caffe-old-unpool/examples/live_net/synset_words.txt", label);
  
	int i=0, j=0, k=0;
	int top5_idx[5];
	float mean_val[3] = {103.939, 116.779, 123.68}; // bgr mean

	// input
	float output[1000];
	vector<Blob<float>*> input_vec;
	Blob<float> blob(1, 3, IMAGE_SIZE, IMAGE_SIZE);

	int crop_size = 256;

	// open video
	IplImage *frame = 0, *crop_image = 0, *small_image = 0;
	CvCapture* capture = cvCaptureFromFile("C:/Users/cht2pal/Desktop/caffe-old-unpool/examples/live_net/basketball.avi");// cvCaptureFromCAM(0);
	crop_image = cvCreateImage(cvSize(crop_size,crop_size), 8, 3);
	small_image = cvCreateImage(cvSize(IMAGE_SIZE, IMAGE_SIZE), 8, 3);

	//cvNamedWindow("Test");
	caffe::Datum datum;
	const char* img_name = "C:/Users/cht2pal/Desktop/caffe-old-unpool/examples/DeconvNet/inference/000079.jpg";
	IplImage *img = cvLoadImage(img_name);
	cvShowImage("raw_image", img);

	cvResize(img, small_image);

	for (k=0; k<3; k++)
	{
		for (i=0; i<IMAGE_SIZE; i++)
		{
			for (j=0; j< IMAGE_SIZE; j++)
			{
				blob.mutable_cpu_data()[blob.offset(0, k, i, j)] = (float)(unsigned char)small_image->imageData[i*small_image->widthStep+j*small_image->nChannels+k] - mean_val[k];
			}
		}
	}
	input_vec.push_back(&blob);

	float loss;

	// Do Forward
	const vector<Blob<float>*>& result = caffe_test_net.Forward(input_vec, &loss);

	Blob<float> *out_blob= result[0];
	LOG(INFO) << out_blob->channels() << " " << out_blob->height() << " " 
			  << out_blob->width() << " " << out_blob->num();

	Mat seg_map(IMAGE_SIZE,IMAGE_SIZE, CV_32FC1, Scalar(0,0,0));
	int sum_pix = 0;
	int idx = 0;
	for( int i = 0; i < IMAGE_SIZE*IMAGE_SIZE; i++)
		sum_pix += result[0]->cpu_data()[idx++];
	int thred = (sum_pix / idx);
	cout << thred << endl;
	idx = 0;
	for(int c = 0; c < out_blob->channels(); c++){
		for(int i = 0; i < out_blob->height(); i++){
			for(int j=0; j < out_blob->width(); j++){
				//apply pixel
				seg_map.at<float>(i,j) =  result[0]->cpu_data()[idx++];
				//cout << result[0]->cpu_data()[idx++] << " ";
				/*if(result[0]->cpu_data()[idx++] >= thred)
				seg_map.at<float>(i,j) = 255;
				else
				seg_map.at<float>(i,j) = 0;*/
			}
		}
		cv::Scalar avg,sdv;
		cv::meanStdDev(seg_map, avg, sdv);
		sdv.val[0] = sqrt(seg_map.cols*seg_map.rows*sdv.val[0]*sdv.val[0]);
		cv::Mat image_32f;
		seg_map.convertTo(image_32f,CV_32FC1,1/sdv.val[0],-avg.val[0]/sdv.val[0]);
		seg_map = image_32f * 255;
		Mat	res_map;
		resize(seg_map, res_map, Size(img->width, img->height));

		Mat result(img->height,img->width, CV_32FC3, Scalar(0,0,0));
		img = cvLoadImage(img_name);
		for (int c = 0; c < 3; c++){
			for(int h = 0; h < res_map.rows; h++){
				for(int w = 0; w < res_map.cols; w++){
					//cout << res_map.at<float>(h,w) << endl;
					if(res_map.at<float>(h,w) <  1)
						img->imageData[h*img->widthStep+w*img->nChannels+c]  = 0;
				}
			}
		}
		cvShowImage("Test", img);
		//imshow("map", res_map);
		waitKey();
	}

	

	/* while(1)
	{
		cvGrabFrame(capture);
		
		frame = cvRetrieveFrame(capture);
		cvShowImage("raw_image", frame);
		//cv::waitKey();
		// crop input image
		for(i=0; i < crop_size; i++)
		{
			for(j=0; j < crop_size; j++)
			{
				crop_image->imageData[i*crop_image->widthStep+j*crop_image->nChannels+0] = frame->imageData[i*frame->widthStep+j*frame->nChannels+0];
				crop_image->imageData[i*crop_image->widthStep+j*crop_image->nChannels+1] = frame->imageData[i*frame->widthStep+j*frame->nChannels+1];
				crop_image->imageData[i*crop_image->widthStep+j*crop_image->nChannels+2] = frame->imageData[i*frame->widthStep+j*frame->nChannels+2];
			}
		}

		cvResize(frame, small_image);
		
		for (k=0; k<3; k++)
		{
			for (i=0; i<IMAGE_SIZE; i++)
			{
				for (j=0; j< IMAGE_SIZE; j++)
				{
					blob.mutable_cpu_data()[blob.offset(0, k, i, j)] = (float)(unsigned char)small_image->imageData[i*small_image->widthStep+j*small_image->nChannels+k] - mean_val[k];
				}
			}
		}

		input_vec.push_back(&blob);

		// forward propagation
		float loss;

		const vector<Blob<float>*>& result = caffe_test_net.Forward(input_vec, &loss);

		// copy output
		for(i=0; i<1000; i++)
		{
			output[i] = result[0]->cpu_data()[i];
		}

		get_top5(output, top5_idx);

		draw_output(crop_image, output, top5_idx, label);
		cvShowImage("Test", crop_image);

		if(cvWaitKey(1) == 27)
			break;

		input_vec.clear();
	}

	cvReleaseCapture(&capture);
	cvReleaseImage(&crop_image);
	cvReleaseImage(&small_image);*/

	waitKey();
	return 0;
}
