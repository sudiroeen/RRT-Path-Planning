#include <opencv2/opencv.hpp>
#include <iostream>
#include <cmath>

#define MAX_DIST 500*sqrt(2)

using namespace std;
using namespace cv;

typedef vector<vector<Point> > KONTUR;
typedef vector<vector<Point> > TREE;
typedef vector<Point> PATH;

Mat img(500, 500, CV_8UC3, Scalar::all(255));
Mat img_hasil(500, 500, CV_8UC3, Scalar::all(255));
Mat img_smooth(500, 500, CV_8UC3, Scalar::all(255));
Mat img_hitam(500, 500, CV_8UC3, Scalar::all(0));
bool tekan = false;
vector<Point> obstacle;
TREE res_tree;

string windowName = "RRT - [Rapidly-exploring Random Tree]";

void mouseCb(int event, int x, int y, int flags, void* userdata){
	if(event == EVENT_LBUTTONDOWN){
		tekan = true;
	}

	if(tekan){
		obstacle.emplace_back(Point(x,y));
		circle(img, Point(x,y), 8, Scalar::all(0), -1);
	}

	if(event == EVENT_LBUTTONUP){
		tekan = false;
	}
}

bool check_intersect(const KONTUR& obs_, vector<Point> sf_line){
	Mat zero1 = Mat::zeros(img.size(), CV_8UC1);
	Mat zero2 = Mat::zeros(img.size(), CV_8UC1);

	for(size_t k=0; k<obs_[0].size(); k++){
		circle(zero1, obs_[0][k], 8, uchar(255), -1);
	}
	line(zero2, sf_line[0], sf_line[1], uchar(255), 2);
	line(img_hitam, sf_line[0], sf_line[1], uchar(255), 1);

	Mat res_bit_and; 
	bitwise_and(zero1, zero2, res_bit_and);
	int res_and = (int)sum(res_bit_and)[0];
	if(res_and > 0)
		return true;
	else
		false;
}

void getFixedPathFunc(PATH& toIsi, const PATH& pathFull, const vector<int>& ind_parent_, size_t id_p){
	if(ind_parent_[id_p] == -1){
		toIsi.insert(toIsi.begin(), pathFull[0]);
		return;
	}
	toIsi.insert(toIsi.begin(), pathFull[id_p]);
	getFixedPathFunc(toIsi, pathFull, ind_parent_, ind_parent_[id_p]);
}

PATH RRT(const KONTUR& obstacle_, const Point& start_, const Point& finish_){
	PATH topath;
	topath.emplace_back(start_);
	topath.emplace_back(finish_);

	vector<int> ind_parent;
	ind_parent.emplace_back(-1);

	if(!check_intersect(obstacle_, topath)){
		cout << "langsung return" << endl;
		line(img, start_, finish_, Scalar(45, 150, 44), 2);
		return topath;
	}

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> rand_val(0,500);
	while(true){
		int rand_x = rand_val(gen);
		int rand_y = rand_val(gen);
		// circle(img, Point(rand_x, rand_y), 4, Scalar(0,0,255), -1);
		float dist0 = sqrt(pow(float(start_.x - rand_x), 2) + pow(float(start_.y - rand_y), 2));
		size_t before_end = topath.size()-1;
		int parent_id = 0;
		for(size_t i=0; i<before_end; i++){
			vector<Point> temp_sfline;
			temp_sfline.emplace_back(topath[i]);
			temp_sfline.emplace_back(Point(rand_x, rand_y));
			if(!check_intersect(obstacle_, temp_sfline)){
				float dist_to_rand = sqrt(pow(float(topath[i].x - rand_x), 2.0) + pow(float(topath[i].y - rand_y),2.0));
				if(dist_to_rand <= dist0){
					parent_id = i;
					dist0 = dist_to_rand;
				}
			}else{
				parent_id = -1;
			}
		}
		if(parent_id != -1){
			line(img, topath[parent_id], Point(rand_x, rand_y), Scalar(45,150,44), 2);
			topath.insert(topath.end()-1, Point(rand_x, rand_y));
			ind_parent.emplace_back(parent_id);
			vector<Point> temp_sfline;
			temp_sfline.emplace_back(finish_);
			temp_sfline.emplace_back(Point(rand_x, rand_y));
			if(!check_intersect(obstacle_, temp_sfline)){
				ind_parent.emplace_back(before_end);
				line(img, topath[before_end], finish_, Scalar(45,150,44), 2);
				break;
			}
		}
		imshow(windowName, img);
		waitKey(1);
	}

	for(size_t s=0; s<topath.size(); s++){
		cout << s << "\t" << topath[s] << "\t" << ind_parent[s] << endl;
	}
	PATH fixed_path;
	fixed_path.emplace_back(finish_);
	getFixedPathFunc(fixed_path, topath, ind_parent, ind_parent[ind_parent.size() - 1]);
	for(size_t k=1; k<fixed_path.size(); k++){
		line(img_hasil, fixed_path[k-1], fixed_path[k], Scalar(255,255,0), 2);
	}
	imshow("path_hasil", img_hasil);
	return fixed_path;
}

PATH BezierSpline(PATH fixed_path_){
	// -1.0000e+00, 0, 0
	// -1.4901e-08, +5.2500e-01, 0
	// +1.0000e+00, -5.0000e-01, 0
	// +1.5000e+00, +4.5000e-01, 0
	PATH smoothedPath;
	fixed_path_.insert(fixed_path_.begin(), Point(fixed_path_[0].x+1, fixed_path_[0].y+1));
	fixed_path_.insert(fixed_path_.end(), Point(fixed_path_[fixed_path_.size()-2].x+1, fixed_path_[fixed_path_.size()-2].y+1));
	Mat M_Bezier = (Mat_<float>(4,4) << -1, 3, -3, 1,
										 3, -6, 3, 0,
										 -3, 3, 0, 0,
										 1, 0, 0, 0);
	float max_step_u = 15.0;
	bool isAwal = true;
	for(size_t s=1; s<fixed_path_.size()-2; s++){
		Mat Posisi = (Mat_<float>(4,2) << fixed_path_[s-1].x, fixed_path_[s-1].y,
										  fixed_path_[s].x, fixed_path_[s].y,
										  fixed_path_[s+1].x, fixed_path_[s+1].y,
										  fixed_path_[s+2].x, fixed_path_[s+2].y);
		if(isAwal){
			for(float step_u=0.0; step_u<=max_step_u; step_u+=1.0){
				float u = step_u/max_step_u;
				Mat seq_u = (Mat_<float>(1,4) << pow(u, 3.0), pow(u, 2.0), u, 1.0);
				Mat hasil_ = seq_u * M_Bezier * Posisi;
				smoothedPath.emplace_back(Point(hasil_.at<float>(0), hasil_.at<float>(1)));
			}
			isAwal = false;
		}else{
			for(float step_u=10.0; step_u<=max_step_u; step_u+=1.0){
				float u = step_u/max_step_u;
				Mat seq_u = (Mat_<float>(1,4) << pow(u, 3.0), pow(u, 2.0), u, 1.0);
				Mat hasil_ = seq_u * M_Bezier * Posisi;
				smoothedPath.emplace_back(Point(hasil_.at<float>(0), hasil_.at<float>(1)));
			}
		}
	}

	for(size_t r=1; r<smoothedPath.size(); r++){
		// cout << "smoothedPath: " << smoothedPath[r] << endl;
		line(img_smooth, smoothedPath[r-1], smoothedPath[r], Scalar::all(0), 2);
	}
	return smoothedPath;
}

int main(){
  Point start = Point(50,50);
  Point finish = Point(450,450);

  res_tree.resize(1);
  res_tree[0].emplace_back(start);
  res_tree[0].emplace_back(finish);
  namedWindow(windowName, CV_WINDOW_NORMAL);
  circle(img, start, 5, Scalar(255,0,0), -1);
  circle(img, finish, 5, Scalar(191,0,255), -1);
  circle(img_hasil, start, 5, Scalar(255,0,0), -1);
  circle(img_hasil, finish, 5, Scalar(191,0,255), -1);

  setMouseCallback(windowName, mouseCb, NULL);

  while(true){
  	imshow(windowName, img);
  	if(waitKey(1) == 27)
  		break;
  }

  PATH getPathRRT;
  if(obstacle.size() != 0){
	  vector<vector<Point> > obstacle_v;
	  obstacle_v.emplace_back(obstacle);
	  getPathRRT = RRT(obstacle_v, start, finish);
	  cout << "getPathRRT: " << endl;
	  for(size_t k=0; k<getPathRRT.size(); k++)
	  	cout << getPathRRT[k] << endl;
	  // PATH smooth_path = BezierSpline(getPathRRT);
	  // imshow("smoothed", img_smooth);
	  imshow(windowName, img);
	}
  waitKey(0);
}