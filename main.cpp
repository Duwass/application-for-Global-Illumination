// main.cpp
//
// Một chương trình C++ đơn giản để so sánh đồ họa Rasterization và Ray Tracing.
//
// Hướng dẫn biên dịch và chạy (mở Terminal hoặc Command Prompt):
// 1. Dùng g++ (Linux/macOS/MinGW trên Windows):
//    g++ main.cpp -o renderer -O3 -std=c++17
//
// 2. Dùng trình biên dịch của Visual Studio (mở "Developer Command Prompt"):
//    cl main.cpp /EHsc /O2
//
// 3. Chạy chương trình:
//    ./renderer  (trên Linux/macOS)
//    renderer.exe (trên Windows)
//
// Chương trình sẽ tạo ra hai file ảnh: "rasterized_scene.ppm" và "raytraced_scene.ppm".

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <limits>
#include <memory>
#include <random>
#include <algorithm> 

// 1: CÁC CẤU TRÚC DỮ LIỆU VÀ HÀM TIỆN ÍCH CƠ BẢN

const double infinity = std::numeric_limits<double>::infinity();
const double PI = 3.1415926535897932385;

inline double random_double() {
    static std::uniform_real_distribution<double> distribution(0.0, 1.0);
    static std::mt19937 generator;
    return distribution(generator);
}

// Cấu trúc vector 3 chiều, dùng cho điểm, vector, và màu sắc.
struct Vec3 {
    double x = 0, y = 0, z = 0;

    Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3& operator+=(const Vec3 &v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vec3& operator*=(const double t) { x *= t; y *= t; z *= t; return *this; }
    Vec3& operator/=(const double t) { return *this *= 1/t; }

    double length_squared() const { return x*x + y*y + z*z; }
    double length() const { return sqrt(length_squared()); }
};

// Các hàm toán tử cho Vec3
inline Vec3 operator+(const Vec3 &u, const Vec3 &v) { return {u.x + v.x, u.y + v.y, u.z + v.z}; }
inline Vec3 operator-(const Vec3 &u, const Vec3 &v) { return {u.x - v.x, u.y - v.y, u.z - v.z}; }
inline Vec3 operator*(const Vec3 &u, const Vec3 &v) { return {u.x * v.x, u.y * v.y, u.z * v.z}; }
inline Vec3 operator*(double t, const Vec3 &v) { return {t*v.x, t*v.y, t*v.z}; }
inline Vec3 operator*(const Vec3 &v, double t) { return t * v; }
inline Vec3 operator/(Vec3 v, double t) { return (1/t) * v; }

inline double dot(const Vec3 &u, const Vec3 &v) { return u.x * v.x + u.y * v.y + u.z * v.z; }
inline Vec3 cross(const Vec3 &u, const Vec3 &v) { return {u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x}; }
inline Vec3 unit_vector(Vec3 v) { return v / v.length(); }
Vec3 reflect(const Vec3& v, const Vec3& n) { return v - 2*dot(v,n)*n; }

// Cấu trúc Tia sáng
struct Ray {
    Vec3 origin;
    Vec3 direction;
};

// Cấu trúc lưu thông tin va chạm
struct HitRecord {
    Vec3 p;
    Vec3 normal;
    double t;
    bool front_face;
    int object_id;

    inline void set_face_normal(const Ray& r, const Vec3& outward_normal) {
        front_face = dot(r.direction, outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

// Lớp trừu tượng cho các vật thể có thể va chạm
class Hittable {
public:
    virtual ~Hittable() = default;
    virtual bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const = 0;
};

// Lớp vật thể hình cầu
class Sphere : public Hittable {
public:
    Sphere(Vec3 cen, double r, int id) : center(cen), radius(r), id(id) {}

    bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const override {
        Vec3 oc = r.origin - center;
        auto a = r.direction.length_squared();
        auto half_b = dot(oc, r.direction);
        auto c = oc.length_squared() - radius*radius;
        auto discriminant = half_b*half_b - a*c;

        if (discriminant < 0) return false;
        auto sqrtd = sqrt(discriminant);

        auto root = (-half_b - sqrtd) / a;
        if (root < t_min || t_max < root) {
            root = (-half_b + sqrtd) / a;
            if (root < t_min || t_max < root)
                return false;
        }

        rec.t = root;
        rec.p = r.origin + rec.t * r.direction;
        Vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.object_id = id;

        return true;
    }

private:
    Vec3 center;
    double radius;
    int id;
};

// Hàm ghi màu một pixel ra file
void write_color(std::ostream &out, Vec3 pixel_color, int samples_per_pixel) {
    auto r = pixel_color.x;
    auto g = pixel_color.y;
    auto b = pixel_color.z;

    // Chia cho số mẫu và gamma-correct for gamma=2.0.
    auto scale = 1.0 / samples_per_pixel;
    r = sqrt(scale * r);
    g = sqrt(scale * g);
    b = sqrt(scale * b);

    // Ghi giá trị màu [0,255]
    out << static_cast<int>(256 * std::clamp(r, 0.0, 0.999)) << ' '
        << static_cast<int>(256 * std::clamp(g, 0.0, 0.999)) << ' '
        << static_cast<int>(256 * std::clamp(b, 0.0, 0.999)) << '\n';
}


//2: BỘ KẾT XUẤT RAY TRACING

// Hàm tính màu sắc của một tia sáng, đây là trái tim của Ray Tracer
Vec3 ray_color(const Ray& r, const std::vector<std::shared_ptr<Hittable>>& world, int depth) {
    HitRecord rec;

    // Nếu tia đã nảy bật quá nhiều lần, không thu thêm ánh sáng.
    if (depth <= 0)
        return {0,0,0};

    // Tìm va chạm gần nhất
    HitRecord temp_rec;
    bool hit_anything = false;
    auto closest_so_far = infinity;

    for (const auto& object : world) {
        if (object->hit(r, 0.001, closest_so_far, temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
        }
    }

    if (hit_anything) {
        Ray scattered;
        Vec3 attenuation;
        
        // Dựa vào ID vật thể để quyết định vật liệu
        switch(rec.object_id) {
            case 0: // Sàn (Vật liệu khuếch tán)
            {
                auto scatter_direction = rec.normal + unit_vector(Vec3{random_double(), random_double(), random_double()});
                scattered = {rec.p, scatter_direction};
                attenuation = {0.5, 0.5, 0.5}; // Màu xám
                return attenuation * ray_color(scattered, world, depth-1);
            }
            case 1: // Quả cầu trung tâm (Vật liệu khuếch tán)
            {
                auto scatter_direction = rec.normal + unit_vector(Vec3{random_double(), random_double(), random_double()});
                scattered = {rec.p, scatter_direction};
                attenuation = {0.7, 0.3, 0.3}; // Màu đỏ nhạt
                return attenuation * ray_color(scattered, world, depth-1);
            }
            case 2: // Quả cầu kim loại
            {
                Vec3 reflected = reflect(unit_vector(r.direction), rec.normal);
                scattered = {rec.p, reflected};
                attenuation = {0.8, 0.8, 0.8}; // Màu bạc
                if (dot(scattered.direction, rec.normal) > 0)
                    return attenuation * ray_color(scattered, world, depth-1);
                else
                    return {0,0,0};
            }
            case 3: // Quả cầu thủy tinh (đơn giản hóa thành kim loại mờ)
            {
                 Vec3 reflected = reflect(unit_vector(r.direction), rec.normal);
                 scattered = {rec.p, reflected + 0.3*unit_vector(Vec3{random_double(), random_double(), random_double()})};
                 attenuation = {0.8, 0.6, 0.2}; // Màu vàng
                 if (dot(scattered.direction, rec.normal) > 0)
                    return attenuation * ray_color(scattered, world, depth-1);
                 else
                    return {0,0,0};
            }
        }
    }

    // Nếu tia không va vào đâu, trả về màu nền bầu trời
    Vec3 unit_direction = unit_vector(r.direction);
    auto t = 0.5*(unit_direction.y + 1.0);
    return (1.0-t)*Vec3{1.0, 1.0, 1.0} + t*Vec3{0.5, 0.7, 1.0};
}


void render_raytraced(int image_width, int image_height, const std::vector<std::shared_ptr<Hittable>>& scene) {
    std::ofstream outfile("raytraced_scene.ppm");
    outfile << "P3\n" << image_width << " " << image_height << "\n255\n";

    std::cout << "Bắt đầu kết xuất Ray Tracing...\n";

    // Các thông số camera
    auto aspect_ratio = (double)image_width / image_height;
    auto viewport_height = 2.0;
    auto viewport_width = aspect_ratio * viewport_height;
    auto focal_length = 1.0;

    auto origin = Vec3{0, 0, 0};
    auto horizontal = Vec3{viewport_width, 0, 0};
    auto vertical = Vec3{0, viewport_height, 0};
    auto lower_left_corner = origin - horizontal/2 - vertical/2 - Vec3{0, 0, focal_length};

    int samples_per_pixel = 100;
    int max_depth = 50;

    for (int j = image_height-1; j >= 0; --j) {
        std::cerr << "\rDòng còn lại: " << j << ' ' << std::flush;
        for (int i = 0; i < image_width; ++i) {
            Vec3 pixel_color{0,0,0};
            for (int s = 0; s < samples_per_pixel; ++s) {
                auto u = (i + random_double()) / (image_width-1);
                auto v = (j + random_double()) / (image_height-1);
                Ray r = {origin, lower_left_corner + u*horizontal + v*vertical - origin};
                pixel_color += ray_color(r, scene, max_depth);
            }
            write_color(outfile, pixel_color, samples_per_pixel);
        }
    }
    std::cerr << "\nHoàn thành Ray Tracing.\n";
}


//3: BỘ KẾT XUẤT RASTERIZATION

// Hàm tính toán chiếu sáng đơn giản (Lambertian) cho Rasterizer
Vec3 simple_lighting(const HitRecord& rec) {
    Vec3 light_pos = {5, 5, -5}; // Vị trí nguồn sáng
    Vec3 light_dir = unit_vector(light_pos - rec.p);
    
    // Tính toán cường độ sáng khuếch tán
    double diff = std::max(0.0, dot(rec.normal, light_dir));
    Vec3 diffuse_color;

    // Lấy màu dựa trên ID vật thể
    switch(rec.object_id) {
        case 0: diffuse_color = {0.5, 0.5, 0.5}; break; // Sàn
        case 1: diffuse_color = {0.7, 0.3, 0.3}; break; // Cầu đỏ
        case 2: diffuse_color = {0.8, 0.8, 0.8}; break; // Cầu bạc
        case 3: diffuse_color = {0.8, 0.6, 0.2}; break; // Cầu vàng
        default: diffuse_color = {1,0,1}; break; // Màu lỗi
    }

    Vec3 ambient = 0.1 * diffuse_color; // Ánh sáng môi trường yếu
    return ambient + diff * diffuse_color;
}

void render_rasterized(int image_width, int image_height, const std::vector<std::shared_ptr<Hittable>>& scene) {
    std::ofstream outfile("rasterized_scene.ppm");
    outfile << "P3\n" << image_width << " " << image_height << "\n255\n";

    std::cout << "Bắt đầu kết xuất Rasterization...\n";

    // Camera setup (tương tự Ray Tracer)
    auto aspect_ratio = (double)image_width / image_height;
    auto viewport_height = 2.0;
    auto viewport_width = aspect_ratio * viewport_height;
    auto focal_length = 1.0;

    auto origin = Vec3{0, 0, 0};
    auto horizontal = Vec3{viewport_width, 0, 0};
    auto vertical = Vec3{0, viewport_height, 0};
    auto lower_left_corner = origin - horizontal/2 - vertical/2 - Vec3{0, 0, focal_length};

    for (int j = image_height-1; j >= 0; --j) {
        std::cerr << "\rDòng còn lại: " << j << ' ' << std::flush;
        for (int i = 0; i < image_width; ++i) {
            auto u = (double)i / (image_width-1);
            auto v = (double)j / (image_height-1);
            Ray r = {origin, lower_left_corner + u*horizontal + v*vertical - origin};

            HitRecord rec;
            bool hit_anything = false;
            double closest_so_far = infinity;
            HitRecord final_rec;

            // Vòng lặp này mô phỏng Z-buffer: tìm vật thể gần nhất với camera
            for (const auto& object : scene) {
                if (object->hit(r, 0.001, closest_so_far, rec)) {
                    hit_anything = true;
                    closest_so_far = rec.t;
                    final_rec = rec;
                }
            }

            Vec3 pixel_color;
            if (hit_anything) {
                // Áp dụng mô hình chiếu sáng đơn giản. Không có bóng đổ, không phản chiếu.
                pixel_color = simple_lighting(final_rec);
            } else {
                // Màu nền
                pixel_color = {0.5, 0.7, 1.0};
            }
            
            // Rasterizer không cần nhiều mẫu, nên ta truyền 1
            write_color(outfile, pixel_color, 1);
        }
    }
    std::cerr << "\nHoàn thành Rasterization.\n";
}

//4: MAIN VÀ DỰNG CẢNH

int main() {
    // Thông số ảnh
    const auto aspect_ratio = 16.0 / 9.0;
    const int image_width = 400;
    const int image_height = static_cast<int>(image_width / aspect_ratio);

    // Tạo cảnh (World)
    std::vector<std::shared_ptr<Hittable>> scene;
    scene.push_back(std::make_shared<Sphere>(Vec3{0, -100.5, -1}, 100, 0)); // Sàn
    scene.push_back(std::make_shared<Sphere>(Vec3{0, 0, -1}, 0.5, 1));      // Cầu trung tâm
    scene.push_back(std::make_shared<Sphere>(Vec3{-1.0, 0.0, -1.0}, 0.5, 2)); // Cầu kim loại
    scene.push_back(std::make_shared<Sphere>(Vec3{1.0, 0.0, -1.0}, 0.5, 3)); // Cầu "thủy tinh" mờ

    // Chạy cả hai bộ kết xuất
    render_rasterized(image_width, image_height, scene);
    render_raytraced(image_width, image_height, scene);

    std::cout << "\nHoàn tất! Kiểm tra file 'rasterized_scene.ppm' và 'raytraced_scene.ppm'.\n";
    // run 2 file ở phần mềm GIMP do có đuôi là ppm
    return 0;
}
