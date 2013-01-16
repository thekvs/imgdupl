#ifndef __DCT_PERCEPTUAL_HASHER_HPP_INCLUDED__
#define __DCT_PERCEPTUAL_HASHER_HPP_INCLUDED__

#include <vector>
#include <string>

#include <stdint.h>

#include <Magick++.h>
#include <Eigen/Dense>

#include "phash.hpp"

namespace imgdupl {

template<int N, int S=8>
class DCTHasher {
public:

    DCTHasher()
    {
        make_dct_matrix();
        dct_t = dct.transpose();
    }

    ~DCTHasher() {}

    std::pair<bool, PHash> hash(const Magick::Image &original_image) const
    {
        PHash phash;
        bool  status = true;

        try {
            phash = hash_impl(original_image);
        } catch (Magick::Exception &e) {
            status = false;
        }

        return std::make_pair(status, phash);
    }

private:

    typedef Eigen::Matrix<float, N, N> DCTMatrix;
    typedef Eigen::Matrix<float, N, N> ImgMatrix;

    DCTMatrix dct;
    DCTMatrix dct_t;

    void make_dct_matrix()
    {
        unsigned int i, k, rows, cols;

        cols = dct.cols();
        rows = dct.rows();

        float n = static_cast<float>(dct.cols());

        for (i = 0; i < cols; i++) {
            dct(0, i) = sqrt(1.0 / n);
        }

        const float c = sqrt(2.0 / n);

        for (k = 1; k < rows; k++) {
            for (i = 0; i < cols; i++) {
                dct(k, i) = c * cos((M_PI / (2 * n)) * k * (2 * i + 1));
            }
        }       
    }

    PHash hash_impl(const Magick::Image &image_) const
    {
        Magick::Image image(image_);

        image.type(Magick::GrayscaleType);

        Magick::Geometry geometry(N, N);
        geometry.aspect(true);

        image.transform(geometry);

        unsigned int width = image.size().width();
        unsigned int height = image.size().height();

        Magick::PixelPacket *pixels = image.getPixels(0, 0, width, height);

        ImgMatrix img;

        for (unsigned int i = 0; i < width; i++) {
            for (unsigned int j = 0; j < height; j++) {
                img(i,j) = static_cast<float>(pixels->red);
                ++pixels;
            }
        }

        DCTMatrix c = dct * img * dct_t;

        Eigen::Matrix<float, S, S> top_left = c.topLeftCorner(S, S);
        Eigen::Matrix<float, S, S> top_left_copy(top_left);

        Eigen::Map< Eigen::Matrix<float, 1, S * S> > v(top_left.data());
        Eigen::Map< Eigen::Matrix<float, 1, S * S> > v_copy(top_left_copy.data());

        std::sort(v_copy.data(), v_copy.data() + v_copy.size());

        float median = (v_copy[v_copy.size() / 2] + v_copy[v_copy.size() / 2 - 1]) / 2.0;

        PHash phash;

        uint64_t hash = 0;
        int      basic_hash_bits_count = sizeof(hash) * 8;
        uint64_t one = 1;

        size_t vsz = v.size();

        for (size_t i = 0; i < vsz; i++) {
            if (v[i] > median) {
                hash |= one;
            }

            one = one << 1;

            if ((i + 1) % basic_hash_bits_count == 0) {
                phash.push_back(hash);
                hash = 0;
                one = 1;
            }
        }

        if (vsz % basic_hash_bits_count != 0) {
            phash.push_back(hash);
        }

        return phash;
    }
};

} // namespace

#endif
