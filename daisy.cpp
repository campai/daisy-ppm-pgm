#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstdlib>

const std::string GRAYSCALE_IMAGE_FORMAT = "P2";
const std::string COLOR_IMAGE_FORMAT = "P3";

const ushort MAXIMUM_IMAGE_WIDTH = 500;
const ushort MAXIMUM_IMAGE_HEIGHT = 500;

struct image {
    uint width;
    uint height;
    ushort colors_number;
    ushort max_color_value;
    uint *data;
};

struct point2d {
    int x;
    int y;
};

struct area2d {
    point2d upper_left;
    point2d lower_right;
};

/*****************************************************************************************
 * Helper functions
 *****************************************************************************************/

bool file_exists(const std::string& name) {
    std::ifstream file(name.c_str());
    bool exists = (file && file.good());
    file.close();

    return exists;
}

image* load_image_file(const std::string image_name) {
    std::ifstream input_image(image_name.c_str());

    char file_magic_number[3] = { '\0' };
    input_image.read(file_magic_number, 2);
    std::string format(file_magic_number);

    ushort colors_number = 0;

    if (format.compare(GRAYSCALE_IMAGE_FORMAT) == 0) {
        colors_number = 1;
    } else if (format.compare(COLOR_IMAGE_FORMAT) == 0) {
        colors_number = 3;
    } else {
        std::cerr << "Unknown image format. Only PPM and PGM formats are supported! Magic number read: " << format << std::endl;
        input_image.close();
        return NULL;
    }

    image *img = new image();
    img->colors_number = colors_number;

    input_image >> img->width;
    input_image >> img->height;
    input_image >> img->max_color_value;

    if (img->width > MAXIMUM_IMAGE_WIDTH || img->height > MAXIMUM_IMAGE_HEIGHT) {
        std::cerr << "Maximum allowed image size is [" << MAXIMUM_IMAGE_WIDTH << ", " << MAXIMUM_IMAGE_HEIGHT << "] pixels, "
        << "but your image is of size [" << img->width << ", " << img->height << "]! Image won't be processed!" << std::endl;
        delete img;
        return NULL;
    }

    long size = img->width * img->height * img->colors_number;
    img->data = new uint[size];

    long read_count = 0;

    while (!input_image.eof()) {
        input_image >> img->data[read_count++];
    }

    read_count--;
    input_image.close();

    if (read_count != size) {
        std::cerr << "Image size (" << size << ") does not match data read (" << read_count << ")" << std::endl;
        delete[] img->data;
        delete img;
        img = NULL;
    }

    return img;
}

void save_image(const image *img, const std::string&file_name) {
    std::ofstream os(file_name.c_str(), std::ofstream::out | std::ofstream::trunc);

    os << (img->colors_number > 1 ? COLOR_IMAGE_FORMAT : GRAYSCALE_IMAGE_FORMAT) << std::endl;
    os << img->width << std::endl;
    os << img->height << std::endl;
    os << img->max_color_value << std::endl;

    long size = img->width * img->height * img->colors_number;

    for (long i = 0; i < size; ++i) {
        os << img->data[i] << std::endl;
    }

    os.close();
}

void read_user_point2d(point2d &point, const std::string &msg) {
    std::cout << msg << "\nX: ";
    std::cin >> point.x;

    std::cout << "Y: ";
    std::cin >> point.y;

    std::cout << std::endl;
}

bool ask_user_for_action(std::string msg, std::string accept_str, std::string refuse_str) {
    std::cout << msg;

    std::string user_answer;
    std::cin >> user_answer;

    //make it all lower case (for case-insensitive comparision)
    std::transform(user_answer.begin(), user_answer.end(), user_answer.begin(), ::tolower);
    std::transform(accept_str.begin(), accept_str.end(), accept_str.begin(), ::tolower);
    std::transform(refuse_str.begin(), refuse_str.end(), refuse_str.begin(), ::tolower);

    while (true) {
        if (user_answer.compare(accept_str) == 0) {
            return true;
        } else if (user_answer.compare(refuse_str) == 0) {
            return false;
        } else {
            std::cout << std::endl << "You can answer only by using [" << accept_str << "] for acceptance, or "
                      << "[" << refuse_str << "] for refusal. Please try again.\n" << std::endl;
            std::cout << msg;
            std::cin >> user_answer;
        }
    }
}

bool validate_zoom_factor(const image* img, const uint zoom_factor) {
    if (zoom_factor <= 0) {
        std::cout << "Zoom facto needs to positive number, greater than 0!\n" << std::endl;
        return false;
    }

    uint new_width = zoom_factor * img->width;
    uint new_height = zoom_factor * img->height;

    if (new_width > MAXIMUM_IMAGE_WIDTH || new_height > MAXIMUM_IMAGE_HEIGHT) {
        std::cout << "\nSpecified zoom factor is too big, resulted image would have been bigger than ["
                  << MAXIMUM_IMAGE_WIDTH << "x" << MAXIMUM_IMAGE_HEIGHT << "]. Try smaller number.\n"
                  << std::endl;
        return false;
    }

    return true;
}

bool validate_user_area(const image *img, area2d& area) {
    if (area.lower_right.x < 0 || area.lower_right.x > img->width
        || area.upper_left.x < 0 || area.upper_left.y > img->height) {

        std::cout << "\nLower right and upper left points should both be within range [0, 0] - ["
                  << img->width << ", " << img->height << "]\n" << std::endl;
        return false;
    }

    return true;
}

inline uint image_point(uint row, uint col, uint depth, image *img) {
	return (row * img->width * img->colors_number) + (col * img->colors_number) + depth;
}

inline void swap(uint& src, uint& dst) {
    if (src != dst) {
        uint tmp = dst;
        dst = src;
        src = tmp;
    }
}

inline void update_image_data(image *img, uint *new_data, const uint width, const uint height) {
    if (img == NULL || img->data == NULL) {
        return;
    }

    delete[] img->data;
    img->width = width;
    img->height = height;
    img->data = new_data;
}

/*****************************************************************************************
 * Main operations functions
 *****************************************************************************************/

void flip_horizontally(image* img) {
    uint src_point, dst_point;

    for (uint row = 0; row < img->height; ++row) {
        for (uint col = 0; col < img->width /2; ++col) {
            for (uint depth = 0; depth < img->colors_number; ++depth) {

                src_point = image_point(row, col, depth, img);
				dst_point = image_point(row, img->width - col - 1, depth, img);

                swap(img->data[src_point], img->data[dst_point]);
            }
        }
    }
}

void flip_vertically(image* img) {
    uint src_point, dst_point;

    for (uint row = 0; row < img->height /2; ++row) {
        for (uint col = 0; col < img->width; ++col) {
            for (uint depth = 0; depth < img->colors_number; ++depth) {

                src_point = image_point(row, col, depth, img);
				dst_point = image_point(img->height - row - 1, col, depth, img);

                swap(img->data[src_point], img->data[dst_point]);
            }
        }
    }
}

void cut_area(image* img, area2d& area) {
    if (area.upper_left.x == 0 && area.upper_left.y == 0
        && area.lower_right.x == img->width && area.lower_right.y == img->height) {
        //no need for cutting it off..
        return;
    }

    uint columns_number = area.lower_right.x - area.upper_left.x;
    uint rows_number = area.lower_right.y - area.upper_left.y;

    uint *new_area = new uint[columns_number * rows_number * img->colors_number];
    uint dst_point, src_point;

    for (uint row = 0; row < rows_number; ++row) {
        for (uint col = 0; col < columns_number; ++col) {
            for (uint depth = 0; depth < img->colors_number; ++depth) {

                dst_point = (row * columns_number * img->colors_number) + (col * img->colors_number) + depth;
                src_point = image_point(area.upper_left.y + row, area.upper_left.x + col, depth, img);

                new_area[dst_point] = img->data[src_point];
            }
        }
    }

    update_image_data(img, new_area, columns_number, rows_number);
}

void zoom_in(image *img, area2d& area, uint zoom_factor) {
	uint columns_number = area.lower_right.x - area.upper_left.x;
	uint rows_number = area.lower_right.y - area.upper_left.y;
	
	uint *new_area = new uint[columns_number * zoom_factor * rows_number * zoom_factor * img->colors_number];
	uint src_index, dst_index;

	for (uint row = 0; row < rows_number; ++row) {
		for (uint col = 0; col < columns_number; ++col) {
			for (uint depth = 0; depth < img->colors_number; ++depth) {

                src_index = image_point(row, col, depth, img);

				for (ushort x = 0; x < zoom_factor; ++x) {
					for (ushort y = 0; y < zoom_factor; ++y) {

                        dst_index = (row * zoom_factor + y) * columns_number * zoom_factor * img->colors_number
                                    + (col * zoom_factor + x) * img->colors_number + depth;

                        new_area[dst_index] = img->data[src_index];
					}
				}

			}					
		}
	}

    update_image_data(img, new_area, columns_number * zoom_factor, rows_number * zoom_factor);
}


/*****************************************************************************************
 * Program entry point
 *****************************************************************************************/
int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: \n\t" << argv[0] << " <PPM or PGM image file>" << std::endl;
        std::exit(-1);
    }

    std::string input_file_name(argv[1]);

    if (!file_exists(input_file_name)) {
        std::cerr << "File \"" << input_file_name << "\" does not exsits!" << std::endl;
        std::exit(-2);
    }

    image *input_img = load_image_file(input_file_name);

    if (input_img == NULL) {
        std::cerr << "Sorry, I couldn't load specified file. Format might be unsupported." << std::endl;
        std::exit(-3);
    }

    std::cout << std::endl << (input_img->colors_number > 1 ? "Color " : "Grayscale ")
              << "image of size [" << input_img->width << "x" << input_img->height
              << "] has been loaded successfuly.\n" << std::endl;

    area2d working_area;

    do {
        std::cout << "Please provide coordiantes of the area you would like to process." << std::endl;

        read_user_point2d(working_area.upper_left, "UPPER LEFT POINT");
        read_user_point2d(working_area.lower_right, "LOWER RIGHT POINT");
    } while (!validate_user_area(input_img, working_area));

    cut_area(input_img, working_area);

    if (ask_user_for_action("Do you want to flip selected area vertically? (Y/N) ", "Y", "N")) {
        std::cout << "Flipping vertically.." << std::endl;
		flip_vertically(input_img);
    }

    if (ask_user_for_action("Do you want to flip selected area horizontally? (Y/N) ", "Y", "N")) {
		std::cout << "Fipping horizontally.." << std::endl;
		flip_horizontally(input_img);
    }

    if (ask_user_for_action("Do you want to zoom in selected area? (Y/N) ", "Y", "N")) {
        uint zoom_factor = 1;

        do {
            std::cout << "How much do you want to zoom-in by (integer number): ";
            std::cin >> zoom_factor;
        } while (!validate_zoom_factor(input_img, zoom_factor));

    	std::cout << "Zooming in.." << std::endl;
		zoom_in(input_img, working_area, zoom_factor);
    }

    save_image(input_img, "result.ppm");

    delete[] input_img->data;
    delete input_img;
}
