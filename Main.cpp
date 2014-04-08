#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>
using namespace std ;

//random generate 1 -1 series with given seed, store to arr.
void rng(int seed, vector<int> &arr){
	srand(seed) ;
	for(int i=0; i<arr.size(); i++)
		//0~499 --> 1, 500~999 --> -1
		arr[i] = rand()%1000>500? 1: -1 ;
}

//input:	original image, in_img
//			watermark, x_arr
//			spread degree, cr
//			embed intensity, alpha
//			key of pn series, key
//output:	embedded image
IplImage* spread_spectrum_embed(IplImage *in_img, vector<int> &x_arr, int cr=10, double alpha=1, int key=0){
	IplImage *embed_img = cvCloneImage(in_img) ;
	vector<int> b_arr, p_arr(x_arr.size()*cr) ;
	//generate pn series by given key
	rng(key, p_arr) ;
	//spread watermark x with spreading degree cr.
	for(int i=0; i<x_arr.size(); i++)
		b_arr.insert(b_arr.end(), cr, x_arr[i]) ;
	//embedding
	int count = 0 ;
	bool even_row = true ;
	for(int r=0; r<embed_img->height; r++){
		for(int c=0; c<embed_img->width; c++){
			double v ;
			//even row scan from left to right
			//odd row scan from right to left
			//embed value = v + alpha*b*p
			/*
				----------------->
				<-----------------
				----------------->
				.
				.
				.
			*/
			if(even_row){
				v = cvGetReal2D(embed_img, r, c) + alpha*b_arr[count]*p_arr[count] ;
				cvSetReal2D(embed_img, r, c, v) ;
			}
			else{
				v = cvGetReal2D(embed_img, r, embed_img->width-c-1) + alpha*b_arr[count]*p_arr[count] ;
				cvSetReal2D(embed_img, r, embed_img->width-c-1, v) ;
			}
			count += 1 ;
			//Finish embed, break for loops.
			//size of pn series may not exactly equal to size of image
			if(count >= b_arr.size()){
				//2 forloops, need to set r to break the outer loop
				r = embed_img->width ;
				break ;
			}
		}
		//switch even to odd, or odd to even.
		even_row = -even_row ;
	}
	return embed_img ;
}

vector<int> spread_spectrum_extract(IplImage* embed_img, int key=0, int cr=10){
	//size of pn series  = x_len * cr
	//In main function, cr is fixed, x_len = size of image / cr
	//x_len is Integer, size of image / cr is float, need to recalculate the size of x_len again.
	int p_arr_len = (embed_img->width * embed_img->height / cr) * cr ;
	vector<int> p_arr(p_arr_len) ;
	rng(key, p_arr) ;
	vector<int> msg ;
	//count for each symbol(0~cr-1)
	//j is use to compute the index of pn series(j*cr+count)
	//sum is the sum of each period, use to define the extract value.
	int count=0, j=0, sum=0 ;
	bool even_row = true ;
	for(int r=0; r<embed_img->height; r++){
		for(int c=0; c<embed_img->width; c++){
			if(even_row){
				sum += cvGetReal2D(embed_img, r, c) * p_arr[j*cr + count] ;
			}
			else{
				sum += cvGetReal2D(embed_img, r, embed_img->width-c-1) * p_arr[j*cr + count] ;
			}
			count += 1 ;
			//end of a period, extract a symbol from sum.
			//reset count, sum to 0.
			if(count >= cr){
				//push extract symbol(define by sum) to msg.
				msg.push_back(sum>0? 1: -1) ;
				count = 0 ;
				sum = 0 ;
				j += 1 ;
				//pn series may be smaller than image size, need to check if end of pn series.
				if(j*cr >= p_arr_len){
					r = embed_img->height ;
					break ;
				}
			}
		}
		even_row = -even_row ;
	}
	return msg ;
}

int main(){
	IplImage *in_img = cvLoadImage("MH1024.bmp", CV_LOAD_IMAGE_GRAYSCALE) ;
	const int cr = 128 ;
	const int x_len = in_img->width * in_img->height / cr ;
	const double alpha = 5 ;
	int key_x, key_y ;
	cout << "key of x: " ;
	cin >> key_x ;
	cout << "key of y: " ;
	cin >> key_y ;
	vector<int> x_arr(x_len) ;
	rng(key_x, x_arr) ;
	IplImage *embed_img = spread_spectrum_embed(in_img, x_arr, cr, alpha, key_y) ;
	vector<int> extract_msg = spread_spectrum_extract(embed_img, key_y, cr) ;

	cout << "Cr: " << cr << endl ;
	cout << "Alpha: " << alpha << endl ;
	int count=0 ;
	for(int i=0; i<x_arr.size(); i++){
		if(x_arr[i] != extract_msg[i])
			count++ ;
	}
	cout << "correction rate: " << (x_len-count)/(double)x_len << endl ;

	cvShowImage("img", in_img) ;
	cvShowImage("embed", embed_img) ;
	cvWaitKey(0) ;
	return 0 ;
}
