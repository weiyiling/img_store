#include <Windows.h>
#include <string>
#include <fstream>
#include <iostream>
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
			
			string path_png(argv[2]);
			wstring path_png_w(path_png.length(), L'');
			copy(path_png.begin(), path_png.end(), path_png_w.begin());
			Gdiplus::Bitmap *pBitmap = new Gdiplus::Bitmap(path_png_w.c_str());
			std::cout << "PNG image info:" <<  std::endl;
			std::cout << "\tWidth:" << pBitmap->GetWidth() << std::endl << "\tHeight:" << pBitmap->GetHeight() << std::endl << "\tPixelFormat:" << std::hex << pBitmap->GetPixelFormat() << std::endl;
			if (pBitmap->GetPixelFormat() == PixelFormat32bppARGB)
			{
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
				byte* pDataPng = (byte*)new byte[bmData.Stride * bmData.Height];
				memset(pDataPng, 0, bmData.Stride * bmData.Height);
				bmData.Scan0 = pDataPng;
				bmData.Reserved = 0;
				pBitmap->LockBits(&rt, Gdiplus::ImageLockMode::ImageLockModeRead | Gdiplus::ImageLockMode::ImageLockModeWrite | Gdiplus::ImageLockMode::ImageLockModeUserInputBuf, pBitmap->GetPixelFormat(), &bmData);

				int count_valid = 0;
				unsigned int* pixel = NULL;
				for (int i=0; i<bmData.Width; i++)
				{
					for (int j=0; j<bmData.Height; j++)
					{
						pixel = (unsigned int*) &pDataPng[(i * bmData.Height + j) * 4];
						if((*pixel & 0xffffff00) == 0x00ffff00)
						{
							count_valid++;
						}
					}
				}
				std::cout << "\tCount_valid:0x" << count_valid << std::endl;

				if (count_valid >= (size_target + 0x10))
				{
					int count_byte_finish = 0;
					bool flag_done = false;
					unsigned int* pixel_last_valid = NULL;
					pixel = NULL;
					for (int i=0; i<bmData.Width; i++)
					{
						for (int j=0; j<bmData.Height; j++)
						{
							pixel = (unsigned int*) &pDataPng[(i * bmData.Height + j) * 4];
							if((*pixel & 0xffffff00) == 0x00ffff00)
							{
								if (count_byte_finish != size_target)
								{
									*pixel = (0x00ffff00 | (p_target[count_byte_finish++] & 0x000000ff));
									pixel_last_valid = pixel;
								}
								else
								{
									if (flag_done)
									{
										break;
									}
									else
									{
										memcpy(pixel_last_valid+1, "-png-argb-encode", 0x10);
										flag_done = true;
									}
								}
							}
						}
					}
					pBitmap->UnlockBits(&bmData);

					Gdiplus::Bitmap *pBitmapNew = new Gdiplus::Bitmap(bmData.Width, bmData.Height, bmData.Stride, bmData.PixelFormat, (byte *)bmData.Scan0);
					CLSID encoderClsid;
					GetEncoderClsid(L"image/png", &encoderClsid); 
					wstring path_png_save(path_png_w);
					path_png_save += L".encode.png";
					pBitmapNew->Save(path_png_save.c_str(), &encoderClsid);
					delete pBitmapNew;

					std::cout << "Sucess finish the task!" << std::endl;
				}
				else
				{
					std::cout << "The png image size is not enough!" << std::endl;
					pBitmap->UnlockBits(&bmData);
				}

				delete pDataPng;
			}
			else
			{
				std::cout << "The png image no include alpha line!" << std::endl;
			}

			delete p_target;
			delete pBitmap;
		}
		else if (memcmp(argv[1], "-d", 2) == 0)
		{
			string path_png(argv[2]);
			wstring path_png_w(path_png.length(), L'');
			copy(path_png.begin(), path_png.end(), path_png_w.begin());
			Gdiplus::Bitmap *pBitmap = new Gdiplus::Bitmap(path_png_w.c_str());
			std::cout << "PNG image info:" <<  std::endl;
			std::cout << "\tWidth:" << pBitmap->GetWidth() << std::endl << "\tHeight:" << pBitmap->GetHeight() << std::endl << "\tPixelFormat:0x" << std::hex << pBitmap->GetPixelFormat() << std::endl;
			if (pBitmap->GetPixelFormat() == PixelFormat32bppARGB)
			{
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
				byte* pDataPng = (byte*)new byte[bmData.Stride * bmData.Height];
				memset(pDataPng, 0, bmData.Stride * bmData.Height);
				bmData.Scan0 = pDataPng;
				bmData.Reserved = 0;
				pBitmap->LockBits(&rt, Gdiplus::ImageLockMode::ImageLockModeRead | Gdiplus::ImageLockMode::ImageLockModeWrite | Gdiplus::ImageLockMode::ImageLockModeUserInputBuf, pBitmap->GetPixelFormat(), &bmData);

				int count_used = -1;
				int count_valid = 0;
				unsigned int* pixel = NULL;
				for (int i=0; i<bmData.Width; i++)
				{
					for (int j=0; j<bmData.Height; j++)
					{
						pixel = (unsigned int*) &pDataPng[(i * bmData.Height + j) * 4];
						if((*pixel & 0xffffff00) == 0x00ffff00)
						{
							count_valid++;
							if (memcmp(pixel+1, "-png-argb-encode", 0x10) == 0)
							{
								count_used = count_valid;
							}
						}
					}
				}
				std::cout << "\tCount_valid:0x" << count_valid << std::endl;
				std::cout << "\tCount_used:0x" << count_used << std::endl;

				if (count_used > 0)
				{
					char *p_target = new char[count_used];
					memset(p_target, 0, count_used);
					
					int count_byte_finish = 0;
					bool flag_done = false;
					unsigned int* pixel_last_valid = NULL;
					pixel = NULL;
					for (int i=0; i<bmData.Width; i++)
					{
						for (int j=0; j<bmData.Height; j++)
						{
							pixel = (unsigned int*) &pDataPng[(i * bmData.Height + j) * 4];
							if((*pixel & 0xffffff00) == 0x00ffff00)
							{
								if (count_byte_finish != count_used)
								{
									p_target[count_byte_finish++] = (*pixel & 0x000000ff);
								}
							}
						}
					}
					
					fstream fs_decode;
					fs_decode.open(path_png + ".decode", fs_decode.binary | fs_decode.out);
					fs_decode.write(p_target, count_used);
					fs_decode.close();

					delete p_target;

					std::cout << "Sucess finish the task!" << std::endl;
				}
				else
				{
					std::cout << "Not encode data find!" << std::endl;
				}

				pBitmap->UnlockBits(&bmData);
				delete pDataPng;
			}
			else
			{
				std::cout << "The png image no include alpha line!" << std::endl;
			}

			delete pBitmap;
		}
		else
		{
			//
		}
	}
	else
	{
		cout << "Tool help\r\n\tpng_argb_store.exe -e test.png target.file\r\n\tpng_argb_store.exe -d test.png" << std::endl;
	}

	Gdiplus::GdiplusShutdown(m_pGdiToken);

	return 0;
}