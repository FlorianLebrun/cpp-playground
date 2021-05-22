#include "./tensor.h"
#include <fstream>
#include <string>

struct BitmapBuffer {

#pragma pack(push, 1)
   struct tFileHeader {
      uint8_t bfTypeChar1 = 'B';
      uint8_t bfTypeChar2 = 'M';
      uint32_t bfSize = 0;
      uint32_t bfReserved = 0;
      uint32_t bfOffBits = 0;
      tFileHeader(uint32_t dataSize = 0) {
         this->bfOffBits = sizeof(tFileHeader) + sizeof(tInfoHeader);
         this->bfSize = this->bfOffBits + dataSize;
      }
   };
   struct tInfoHeader {
      uint32_t biSize = sizeof(tInfoHeader);
      uint32_t biWidth = 0;
      uint32_t biHeight = 0;
      uint16_t biPlanes = 1;
      uint16_t biBitCount = 0;
      uint32_t biCompression = 0;
      uint32_t biSizeImage = 0;
      uint32_t biXPelsPerMeter = 0;
      uint32_t biYPelsPerMeter = 0;
      uint32_t biColorUsed = 0;
      uint32_t biColorImportant = 0;
      tInfoHeader(uint32_t biWidth = 0, uint32_t biHeight = 0, size_t depth = 3)
         : biWidth(biWidth), biHeight(biHeight), biBitCount(depth * 8) {
      }
   };
#pragma pack(pop)

   typedef uint8_t tPixel[3];

   // Data dimension
   size_t width = 0;
   size_t height = 0;
   size_t depth = 0;

   // Data memory
   uint8_t* data = 0;
   size_t pix_size = 0;
   size_t row_size = 0;
   size_t data_size = 0;

   BitmapBuffer(Size& sizing) {
      this->width = sizing.size[0];
      this->height = sizing.size[1];
      this->depth = 3;

      this->pix_size = this->depth * sizeof(uint8_t);
      this->row_size = width * this->pix_size;
      this->row_size += (4 - (this->row_size) % 4) % 4;
      this->data_size = this->row_size * this->height;

      this->data = (uint8_t*)malloc(this->data_size);
      memset(this->data, 0, this->data_size);
   }
   tPixel* operator [](size_t index) {
      if (index >= this->height) return 0;
      return (tPixel*)&this->data[index * this->row_size];
   }
   void writeHeader(std::ofstream& f) {
      tInfoHeader info(this->width, this->height, this->depth);
      tFileHeader header(this->data_size);
      f.write((char*)&header, sizeof(header));
      f.write((char*)&info, sizeof(info));
   }
   void writeData(std::ofstream& f) {
      f.write((char*)this->data, this->data_size);
   }
};

void saveBitmap(std::string path, Tensor* data) {
   checkAssert(data->size.dim == 2);
   BitmapBuffer buffer(data->size);

   size_t i = 0;
   for (int x = 0; x < buffer.width; x++) {
      for (int y = 0; y < buffer.height; y++) {
         Scalar value = data->values[i++];
         if (value >= 0) buffer[x][y][0] = uint8_t(value * 256.0);
         else buffer[x][y][1] = uint8_t(value * -256.0);
      }
   }

   std::ofstream f(path, std::ios_base::trunc | std::ios_base::binary);
   buffer.writeHeader(f);
   buffer.writeData(f);
   f.close();
   exit(0);
}

Scalar randomScalar(Scalar min, Scalar max) {
   return min + (Scalar(rand()) / (Scalar(RAND_MAX) / (max - min)));
}
