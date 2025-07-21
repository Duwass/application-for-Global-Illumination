# Báo cáo Phân tích: Cài đặt và So sánh Rasterization vs. Ray Tracing trong C++

## 1. Giới thiệu và Mục tiêu

[cite_start]Dự án này là một báo cáo phân tích chi tiết về một chương trình C++ được thiết kế như một **bài toán ứng dụng giáo dục**[cite: 90]. [cite_start]Mục tiêu cốt lõi là triển khai và so sánh trực quan hai công nghệ kết xuất đồ họa nền tảng[cite: 90]:

* [cite_start]**Rasterization (Đồ họa Raster hóa):** Kỹ thuật đồ họa truyền thống, là nền tảng cho hầu hết các game và ứng dụng 3D thời gian thực trong nhiều thập kỷ[cite: 91].
* [cite_start]**Ray Tracing (Dò tia):** Kỹ thuật mô phỏng đường đi của ánh sáng dựa trên các nguyên tắc vật lý, được coi là tiêu chuẩn vàng để đạt được hình ảnh chân thực trong đồ họa điện ảnh[cite: 92].

[cite_start]Báo cáo đi sâu vào mã nguồn để làm rõ cách mỗi công nghệ được cài đặt và những khác biệt về chất lượng hình ảnh mà chúng tạo ra từ cùng một cảnh 3D[cite: 93].

## 2. Cấu trúc Chương trình

[cite_start]Mã nguồn được tổ chức một cách logic thành 4 phần chính để dễ dàng theo dõi và phân tích[cite: 95]:

1.  [cite_start]**Phần 1: Nền tảng Toán học và Cấu trúc Dữ liệu:** Định nghĩa các khối xây dựng cơ bản như vector, tia sáng, và các vật thể hình học[cite: 96].
2.  [cite_start]**Phần 2: Bộ kết xuất Ray Tracing:** Chứa logic của thuật toán Path Tracing để tạo ra ảnh `raytraced_scene.ppm`[cite: 97].
3.  [cite_start]**Phần 3: Bộ kết xuất Rasterization:** Chứa logic của thuật toán Rasterization đơn giản để tạo ra ảnh `rasterized_scene.ppm`[cite: 98].
4.  [cite_start]**Phần 4: Hàm `main` và Dựng cảnh:** Điểm khởi đầu của chương trình, nơi cảnh 3D được khởi tạo và các bộ kết xuất được gọi thực thi[cite: 99].

## 3. Phân tích Kỹ thuật

### 3.1. Nền tảng
* [cite_start]**Cấu trúc `Vec3`** được sử dụng linh hoạt để biểu diễn điểm, vector và màu sắc (RGB)[cite: 103, 104, 105, 106]. [cite_start]Việc nạp chồng toán tử giúp mã nguồn trở nên tự nhiên và dễ đọc hơn[cite: 107].
* [cite_start]**Lớp `Hittable`** định nghĩa một giao diện trừu tượng cho các vật thể có thể tương tác với tia sáng thông qua hàm `hit()`[cite: 111, 112].
* [cite_start]**Lớp `Sphere`** là một triển khai cụ thể của `Hittable`, sử dụng công thức toán học để tính toán giao điểm giữa tia và hình cầu[cite: 113, 114].

### 3.2. Bộ kết xuất Ray Tracing (Path Tracing)
* [cite_start]Bộ kết xuất này triển khai thuật toán **Path Tracing**, một dạng nâng cao của Ray Tracing[cite: 117].
* [cite_start]Hàm `ray_color()` là trung tâm của bộ kết xuất[cite: 124]. [cite_start]Nó hoạt động theo cơ chế **đệ quy (recursion)**, với mỗi lần gọi hàm tương ứng với một lần ánh sáng nảy bật[cite: 125, 126]. [cite_start]Tham số `depth` được dùng để giới hạn số lần nảy bật, tránh vòng lặp vô tận[cite: 127].
* [cite_start]**Mô phỏng vật liệu** được thực hiện dựa trên ID của vật thể, cho phép xử lý các bề mặt khuếch tán (bật nảy ngẫu nhiên) và kim loại (phản xạ gương)[cite: 128, 129, 131].
* [cite_start]**Ánh sáng môi trường** được mô phỏng bằng màu nền của bầu trời, đóng vai trò là nguồn sáng gián tiếp cho toàn bộ cảnh[cite: 135, 136, 137].
* [cite_start]**Khử răng cưa (Anti-Aliasing)** được thực hiện bằng cách bắn nhiều tia (100 tia) qua mỗi pixel và lấy trung bình kết quả màu, tạo ra các cạnh vật thể mượt mà hơn[cite: 122, 123].

### 3.3. Bộ kết xuất Rasterization
* [cite_start]Bộ kết xuất này mô phỏng phương pháp Rasterization một cách đơn giản[cite: 139].
* [cite_start]Điểm khác biệt chính là việc sử dụng cơ chế **Z-Buffer giả lập**, trong đó với mỗi pixel, chương trình sẽ xác định vật thể nào nằm gần camera nhất và chỉ hiển thị vật thể đó[cite: 142, 143, 144].
* [cite_start]Hàm `simple_lighting()` triển khai mô hình **chiếu sáng cục bộ (Local Illumination)**[cite: 146]. [cite_start]Nó chỉ tính toán ánh sáng dựa trên vị trí điểm va chạm, pháp tuyến bề mặt và hướng tới một nguồn sáng duy nhất[cite: 147].
* [cite_start]Mô hình này **không có khả năng tạo bóng đổ hoặc phản chiếu** vì nó không nhận biết được sự tồn tại của các vật thể khác trong cảnh và không mô phỏng sự nảy bật của tia sáng[cite: 148, 149].

## 4. Kết luận Phân tích

[cite_start]Chương trình đã cài đặt thành công một bài toán ứng dụng cho phép so sánh trực tiếp hai mô hình đồ họa[cite: 157].

* [cite_start]**Rasterization** được minh họa là một phương pháp cục bộ, chỉ có khả năng tính toán ánh sáng trực tiếp, dẫn đến hình ảnh "phẳng" và thiếu các hiệu ứng vật lý quan trọng[cite: 158].
* [cite_start]**Ray Tracing (Path Tracing)** được minh họa là một phương pháp mô phỏng vật lý toàn diện[cite: 159]. [cite_start]Bằng cách mô phỏng đường đi và sự nảy bật của ánh sáng, nó tự động tạo ra các hiệu ứng phức tạp như bóng đổ mềm, phản chiếu chính xác, và chiếu sáng gián tiếp, mang lại kết quả chân thực hơn nhiều[cite: 160].

[cite_start]Dù đơn giản, chương trình này đã minh họa rõ ràng lý do tại sao Ray Tracing được xem là tương lai của đồ họa máy tính hiện thực[cite: 161].
