#include <Windows.h>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <locale.h> 
#include <gdiplus.h>
#include <atlimage.h>
#include <gdiplusheaders.h>
#include <GdiPlusImaging.h>

using namespace std;

#pragma comment (lib, "Gdiplus.lib")

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT num= 0;
	UINT size= 0;

	Gdiplus::ImageCodecInfo* pImageCodecInfo= NULL;

	Gdiplus::GetImageEncodersSize(&num, &size);
	if(size== 0)
	{
		return -1;
	}
	pImageCodecInfo= (Gdiplus::ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo== NULL)
	{
		return -1;
	}

	GetImageEncoders(num, size, pImageCodecInfo);

	for(UINT j=0; j< num; ++j)
	{
		if(wcscmp(pImageCodecInfo[j].MimeType, format)== 0)
		{
			*pClsid= pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;
		}
	}

	free(pImageCodecInfo);
	return -1;
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, ""); //ÉèÖÃÖÐÎÄÓïÑÔ
	
	Gdiplus::GdiplusStartupInput m_gdiplusStartupInput;
	ULONG_PTR m_pGdiToken;
	Gdiplus::GdiplusStartup(&m_pGdiToken, &m_gdiplusStartupInput, NULL); 

	if (argc >= 3)
	{
		int mode = atoi(argv[1]);
		if (memcmp(argv[1], "-e", 2) == 0)
		{
			string path_target(argv[3]);
			fstream fs_target;
			fs_target.open(path_target, fs_target.binary | fs_target.in);
			fs_target.seekg(0, fs_target.end);
			long long size_target = fs_target.tellg();
			char *p_target = new char[size_target];
			fs_target.seekg(0, fs_target.beg);
			fs_target.read(p_target, size_target);
			fs_target.close();

			char *p_target_bin = new char[(size_target + 4) * 8];
			for (int i=0; i<4; i++)
			{
				for (int j=0; j<8; j++)
				{
					p_target_bin[i*8+j] = ((((byte*)&size_target)[i] >> j) & 0x1);
				}
			}
			for (int i=0; i<size_target; i++)
			{
				for (int j=0; j<8; j++)
				{
					p_target_bin[(i+4)*8+j] = ((p_target[i] >> j) & 0x1);
				}
			}
			
			string path_img(argv[2]);
			wstring path_img_w(path_img.length(), L'');
			copy(path_img.begin(), path_img.end(), path_img_w.begin());
			Gdiplus::Bitmap *pBitmap = new Gdiplus::Bitmap(path_img_w.c_str());
			std::cout << "Image info:" <<  std::endl;
			std::cout << "\tWidth:" << pBitmap->GetWidth() << std::endl << "\tHeight:" << pBitmap->GetHeight() << std::endl << "\tPixelFormat:" << std::hex << pBitmap->GetPixelFormat() << std::endl;
			
			if(((size_target + 4) * 8) <= (pBitmap->GetWidth() * pBitmap->GetHeight()))
			{
				if(pBitmap->GetPixelFormat() != PixelFormat32bppARGB && pBitmap->GetPixelFormat() != PixelFormat32bppRGB)
				{
					std::cout << "Warning:The picture format not support yet, maybe lost some data byte!" <<  std::endl;
				}
				
				Gdiplus::Rect rt;
				rt.X = 0;
				rt.Y = 0;
				rt.Width = pBitmap->GetWidth();
				rt.Height = pBitmap->GetHeight();
				Gdiplus::BitmapData bmData;
				bmData.Width = pBitmap->GetWidth();
				bmData.Height = pBitmap->GetHeight();
				bmData.Stride = 4 * bmData.Width & 0x1FFFFFFF;
				bmData.PixelFormat = pBitmap->GetPixelFormat();
				byte* pDataImg = (byte*)new byte[bmData.Stride * bmData.Height];
				memset(pDataImg, 0, bmData.Stride * bmData.Height);
				bmData.Scan0 = pDataImg;
				bmData.Reserved = 0;
				pBitmap->LockBits(&rt, Gdiplus::ImageLockMode::ImageLockModeRead | Gdiplus::ImageLockMode::ImageLockModeWrite | Gdiplus::ImageLockMode::ImageLockModeUserInputBuf, pBitmap->GetPixelFormat(), &bmData);

				int count_finish = 0;
				for (int i=0; (i<bmData.Width)&&(count_finish<(size_target+4)*8); i++)
				{
					for (int j=0; (j<bmData.Height)&&(count_finish<(size_target+4)*8); j++)
					{
						unsigned int* pixel = NULL;
						pixel = (unsigned int*) &pDataImg[(i * bmData.Height + j) * 4];
						*pixel = ((*pixel & 0xfffffffe) |  p_target_bin[count_finish++]);
					}
				}
				pBitmap->UnlockBits(&bmData);

				Gdiplus::Bitmap *pBitmapNew = new Gdiplus::Bitmap(bmData.Width, bmData.Height, bmData.Stride, bmData.PixelFormat, (byte *)bmData.Scan0);
				wstring path_img_save(path_img_w);
				wstring ext_img_save(path_img_w.substr(path_img_w.rfind(L".") + 1));
				path_img_save += L".encode.";
				path_img_save += ext_img_save;
				CLSID encoderClsid;
				wstring encoderName(L"image/");
				encoderName += ext_img_save;
				transform(encoderName.begin(), encoderName.end(), encoderName.begin(), ::tolower);
				if (encoderName.find(L"jpg") != encoderName.npos)
				{
					encoderName = L"image/jpeg";
				}
				GetEncoderClsid(encoderName.c_str(), &encoderClsid); 
				pBitmapNew->Save(path_img_save.c_str(), &encoderClsid);
				delete pBitmapNew;

				std::cout << "Sucess finish the task!" << std::endl;

				delete pDataImg;
			}
			else
			{
				std::cout << "The picture length is not enough!" << std::endl;
			}

			
			delete p_target;
			delete p_target_bin;
			delete pBitmap;
		}
		else if (memcmp(argv[1], "-d", 2) == 0)
		{
			string path_img(argv[2]);
			wstring path_img_w(path_img.length(), L'');
			copy(path_img.begin(), path_img.end(), path_img_w.begin());
			Gdiplus::Bitmap *pBitmap = new Gdiplus::Bitmap(path_img_w.c_str());
			std::cout << "Image info:" <<  std::endl;
			std::cout << "\tWidth:" << pBitmap->GetWidth() << std::endl << "\tHeight:" << pBitmap->GetHeight() << std::endl << "\tPixelFormat:0x" << std::hex << pBitmap->GetPixelFormat() << std::endl;

			Gdiplus::Rect rt;
			rt.X = 0;
			rt.Y = 0;
			rt.Width = pBitmap->GetWidth();
			rt.Height = pBitmap->GetHeight();
			Gdiplus::BitmapData bmData;
			bmData.Width = pBitmap->GetWidth();
			bmData.Height = pBitmap->GetHeight();
			bmData.Stride = 4 * bmData.Width & 0x1FFFFFFF;
			bmData.PixelFormat = pBitmap->GetPixelFormat();
			byte* pDataImg = (byte*)new byte[bmData.Stride * bmData.Height];
			memset(pDataImg, 0, bmData.Stride * bmData.Height);
			bmData.Scan0 = pDataImg;
			bmData.Reserved = 0;
			pBitmap->LockBits(&rt, Gdiplus::ImageLockMode::ImageLockModeRead | Gdiplus::ImageLockMode::ImageLockModeWrite | Gdiplus::ImageLockMode::ImageLockModeUserInputBuf, pBitmap->GetPixelFormat(), &bmData);
		
			int size_store = 0;
			int count_bin_size = 0;
			//byte size_bin[32] = {0};
			for (int i=0; (i<bmData.Width)&&(count_bin_size<32); i++)
			{
				for (int j=0; (j<bmData.Height)&&(count_bin_size<32); j++)
				{
					unsigned int* pixel = NULL;
					pixel = (unsigned int*) &pDataImg[(i * bmData.Height + j) * 4];
					//size_bin[count_bin_size] = (*pixel & 0x00000001);
					size_store |= ((*pixel & 0x00000001) << count_bin_size);
					count_bin_size++;
				}
			}

			
			std::cout << "\tCount_pixel:0x" << bmData.Width * bmData.Height << std::endl;
			std::cout << "\tSize_store:0x" << size_store << std::endl;

			if ((size_store > 0) && (size_store < bmData.Width * bmData.Height))
			{
				char *p_target = new char[size_store];
				memset(p_target, 0, size_store);
					
				int count_byte_finish = 0;
				bool flag_done = false;
				unsigned int* pixel_last_valid = NULL;
				
				for (int i=0; (i<bmData.Width)&&(count_bin_size<((size_store+4)*8)); i++)
				{
					for (int j=0; (j<bmData.Height)&&(count_bin_size<((size_store+4)*8)); j++)
					{
						if(i * bmData.Height + j < 32)
						{
							continue;
						}
						
						unsigned int* pixel = NULL;
						pixel = (unsigned int*) &pDataImg[(i * bmData.Height + j) * 4];
						p_target[(count_bin_size-32)/8] |= ((*pixel & 0x00000001) << ((count_bin_size-32)%8));

						count_bin_size++;
					}
				}
					
				fstream fs_decode;
				fs_decode.open(path_img + ".decode", fs_decode.binary | fs_decode.out);
				fs_decode.write(p_target, size_store);
				fs_decode.close();

				delete p_target;

				std::cout << "Sucess finish the task!" << std::endl;
			}
			else
			{
				std::cout << "Somewhere is wrong!" << std::endl;
			}

			pBitmap->UnlockBits(&bmData);
			delete pDataImg;

			delete pBitmap;
		}
		else
		{
			//
		}
	}
	else
	{
		cout << "Tool help\r\n\tlsb_store.exe -e test.png target.file\r\n\tlsb_store.exe -d test.png" << std::endl;
	}

	Gdiplus::GdiplusShutdown(m_pGdiToken);

	return 0;
}