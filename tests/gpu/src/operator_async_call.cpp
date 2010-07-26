#include "gputest.hpp"
#include "highgui.h"
#include "cv.h"
#include <string>
#include <iostream>
#include <fstream>
#include <iterator>
#include <limits>
#include <numeric>
#include <iomanip> // for  cout << setw()

using namespace cv;
using namespace std;
using namespace gpu;

class CV_GpuMatASyncCall : public CvTest
{
    public:
        CV_GpuMatASyncCall();
        ~CV_GpuMatASyncCall();
    protected:

        template <typename T>
        void print_mat(const T & mat, const std::string & name) const;

        void run(int);

        bool compare_matrix(cv::Mat & cpumat, gpu::GpuMat & gpumat);

    private:
        int rows;
        int cols;
};

CV_GpuMatASyncCall::CV_GpuMatASyncCall(): CvTest( "GPU-MatOperatorASyncCall", "async" )
{
    rows = 234;
    cols = 123;

    //#define PRINT_MATRIX
}

CV_GpuMatASyncCall::~CV_GpuMatASyncCall() {}

template<typename T>
void CV_GpuMatASyncCall::print_mat(const T & mat, const std::string & name) const
{
    cv::imshow(name, mat);
}

bool CV_GpuMatASyncCall::compare_matrix(cv::Mat & cpumat, gpu::GpuMat & gpumat)
{
    Mat cmat(cpumat.size(), cpumat.type(), Scalar::all(0));
    GpuMat gmat0(cmat);
    GpuMat gmat1;
    GpuMat gmat2;
    GpuMat gmat3;

    //int64 time = getTickCount();

    CudaStream stream;
    stream.enqueueCopy(gmat0, gmat1);
    stream.enqueueCopy(gmat0, gmat2);
    stream.enqueueCopy(gmat0, gmat3);
    stream.waitForCompletion();

    //int64 time1 = getTickCount();

    gmat1.copyTo(gmat0);
    gmat2.copyTo(gmat0);
    gmat3.copyTo(gmat0);

    //int64 time2 = getTickCount();

    //std::cout << "\ntime async: " << std::fixed << std::setprecision(12) << double((time1 - time)  / (double)getTickFrequency());
    //std::cout << "\ntime  sync: " << std::fixed << std::setprecision(12) << double((time2 - time1) / (double)getTickFrequency());
    //std::cout << "\n";

#ifdef PRINT_MATRIX
    print_mat(cmat, "cpu mat");
    print_mat(gmat0, "gpu mat 0");
    print_mat(gmat1, "gpu mat 1");
    print_mat(gmat2, "gpu mat 2");
    print_mat(gmat3, "gpu mat 3");
    cv::waitKey(0);
#endif

    double ret = norm(cmat, gmat0) + norm(cmat, gmat1) + norm(cmat, gmat2) + norm(cmat, gmat3);

    if (ret < 1.0)
        return true;
    else
    {
        std::cout << "return : " << ret << "\n";
        return false;
    }
}

void CV_GpuMatASyncCall::run( int /* start_from */)
{
    bool is_test_good = true;

    Mat cpumat(rows, cols, CV_8U);
    cpumat.setTo(Scalar::all(127));
    GpuMat gpumat(cpumat);
    is_test_good &= compare_matrix(cpumat, gpumat);

    if (is_test_good == true)
        ts->set_failed_test_info(CvTS::OK);
    else
        ts->set_failed_test_info(CvTS::FAIL_GENERIC);
}

CV_GpuMatASyncCall CV_GpuMatASyncCall_test;
