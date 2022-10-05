#include <Windows.h>
#include <sdkddkver.h>
#include <commdlg.h>
#define _WIN32_WINNT _NT_TARGET_VERSION_VISTA
#define WINVER _NT_TARGET_VERSION_VISTA

#include <opencv2/opencv.hpp>
#include <qrencode.h>

#define PARAMETROCANNY 250
#define BUFFER_SIZE 1000

#define PROC_INFO_MENU 1
#define PROC_OPEN_FILE 2
#define PROC_GENERATE_HTQR 3
#define PROC_GENERATE_HIST 4
#define PROC_SAVE_FILE 5
#define PROC_EXIT_MENU 6
#define PROC_GENERATE_QR 7
#define PROC_IMG_HALFTONE 8
#define BACKGROUND_COLOUR 205

using namespace std;
using namespace cv;

// Global variables
HWND editBox;

void halftone(Mat &outImg)
{
    int M = outImg.rows;
    int N = outImg.cols;

    double T = 127.5; /* Threshold */
    Mat auxImg = outImg.clone();
    Mat y = outImg.clone();
    int error = 0;
    int aux = 0;
    int rows, cols;

    for (rows = 0; rows < M - 1; rows++) /* Left Boundary of Image */
    {
        if (y.at<uchar>(rows, 0) >= T)
            aux = 1;

        else
            aux = 0;

        auxImg.at<uchar>(rows, 0) = 255 * aux;
        error = -1 * auxImg.at<uchar>(rows, 0) + y.at<uchar>(rows, 0);

        y.at<uchar>(rows, 0 + 1) = (7 / 16) * error + y.at<uchar>(rows, 0 + 1);
        y.at<uchar>(rows + 1, 0 + 1) = (1 / 16) * error + y.at<uchar>(rows + 1, 0 + 1);
        y.at<uchar>(rows + 1, 0) = (5 / 16) * error + y.at<uchar>(rows + 1, 0);

        for (cols = 1; cols < N - 1; cols++) /* Center of Image */
        {
            if (y.at<uchar>(rows, cols) >= T)
                aux = 1;

            else
                aux = 0;

            auxImg.at<uchar>(rows, cols) = 255 * aux;
            error = -1 * auxImg.at<uchar>(rows, cols) + y.at<uchar>(rows, cols);
            y.at<uchar>(rows, cols + 1) = (7 / 16) * error + y.at<uchar>(rows, cols + 1);
            y.at<uchar>(rows + 1, cols + 1) = (1 / 16) * error + y.at<uchar>(rows + 1, cols + 1);
            y.at<uchar>(rows + 1, cols) = (5 / 16) * error + y.at<uchar>(rows + 1, cols);
            y.at<uchar>(rows + 1, cols - 1) = (3 / 16) * error + y.at<uchar>(rows + 1, cols - 1);
        }

        /* Right Boundary of Image */
        if (y.at<uchar>(rows, N - 1) >= T)
            aux = 1;

        else
            aux = 0;

        auxImg.at<uchar>(rows, N - 1) = 255 * aux;
        error = -1 * auxImg.at<uchar>(rows, N - 1) + y.at<uchar>(rows, N - 1);
        y.at<uchar>(rows + 1, N - 1) = (5 / 16) * error + y.at<uchar>(rows + 1, N - 1);
        y.at<uchar>(rows + 1, N - 2) = (3 / 16) * error + y.at<uchar>(rows + 1, N - 2);
    }

    /* Bottom & Left Boundary of Image */

    rows = M - 1;

    if (y.at<uchar>(rows, 0) >= T)
        aux = 1;

    else
        aux = 0;


    auxImg.at<uchar>(rows, 0) = 255 * aux;
    error = -1 * auxImg.at<uchar>(rows, 0) + y.at<uchar>(rows, 0);
    y.at<uchar>(rows, 0 + 1) = (7 / 16) * error + y.at<uchar>(rows, 0 + 1);

    for (cols = 1; cols < N - 1; cols++) /* Bottom & Center of Image */
    {
        if (y.at<uchar>(rows, cols) >= T)
            aux = 1;

        else
            aux = 0;

        auxImg.at<uchar>(rows, cols) = 255 * aux;
        error = -1 * auxImg.at<uchar>(rows, cols) + y.at<uchar>(rows, cols);
        y.at<uchar>(rows, cols + 1) = (7 / 16) * error + y.at<uchar>(rows, cols + 1);
    }

    /* Thresholding */

    if (y.at<uchar>(rows, N - 1) >= T)
        aux = 1;

    else
        aux = 0;

    auxImg.at<uchar>(rows, N - 1) = 255 * aux;
    outImg = auxImg;
}

void clear_qrcode(const Mat qr_in, Mat &qr_out) // limpar tudo que nao e essencial no qrcode
{
	Mat qr_aux(qr_in.size(), qr_in.type());
	Mat qr_aux2 = qr_aux.clone();
	int rows = qr_in.rows;
	int cols = qr_in.cols;
	if (!(rows && cols)) {
		return;
	}
	int square_size = 0;
	int k = 0;
	while (qr_in.at<uchar>(k,0) == 0) {
		square_size++;
		k++;
	}
	square_size++;

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			if ((i < square_size && j < square_size) ||
				(i < square_size && j >= rows - square_size) ||
				(i >= cols - square_size && j < square_size) ||
				(i == square_size-2) || (j == square_size-2)) {
				qr_aux.at<uchar>(i, j) = qr_in.at<uchar>(i, j);
			}
		}
	}
	// find small squares and copy
	for (int i = 2; i < rows-2; i++) {
		for (int j = 2; j < cols-2; j++) {
			if (!qr_in.at<uchar>(i-2,j-2) && !qr_in.at<uchar>(i-2,j-1) && !qr_in.at<uchar>(i-2,j) && !qr_in.at<uchar>(i-2,j+1) && !qr_in.at<uchar>(i-2,j+2) &&
				!qr_in.at<uchar>(i-1,j-2) &&  qr_in.at<uchar>(i-1,j-1) &&  qr_in.at<uchar>(i-1,j) &&  qr_in.at<uchar>(i-1,j+1) && !qr_in.at<uchar>(i-1,j+2) &&
				!qr_in.at<uchar>(i,j-2)   &&  qr_in.at<uchar>(i,j-1)   && !qr_in.at<uchar>(i,j)   &&  qr_in.at<uchar>(i,j+1)   && !qr_in.at<uchar>(i,j+2)   &&
				!qr_in.at<uchar>(i+1,j-2) &&  qr_in.at<uchar>(i+1,j-1) &&  qr_in.at<uchar>(i+1,j) &&  qr_in.at<uchar>(i+1,j+1) && !qr_in.at<uchar>(i+1,j+2) &&
				!qr_in.at<uchar>(i+2,j-2) && !qr_in.at<uchar>(i+2,j-1) && !qr_in.at<uchar>(i+2,j) && !qr_in.at<uchar>(i+2,j+1) && !qr_in.at<uchar>(i+2,j+2)) {

				qr_aux.at<uchar>(i-2,j-2) = qr_in.at<uchar>(i-2,j-2);
				qr_aux.at<uchar>(i-2,j-1) = qr_in.at<uchar>(i-2,j-1);
				qr_aux.at<uchar>(i-2,j)   = qr_in.at<uchar>(i-2,j)  ;
				qr_aux.at<uchar>(i-2,j+1) = qr_in.at<uchar>(i-2,j+1);
				qr_aux.at<uchar>(i-2,j+2) = qr_in.at<uchar>(i-2,j+2);

				qr_aux.at<uchar>(i-1,j-2) = qr_in.at<uchar>(i-1,j-2);
				qr_aux.at<uchar>(i-1,j-1) = qr_in.at<uchar>(i-1,j-1);
				qr_aux.at<uchar>(i-1,j)   = qr_in.at<uchar>(i-1,j)  ;
				qr_aux.at<uchar>(i-1,j+1) = qr_in.at<uchar>(i-1,j+1);
				qr_aux.at<uchar>(i-1,j+2) = qr_in.at<uchar>(i-1,j+2);

				qr_aux.at<uchar>(i,j-2)   = qr_in.at<uchar>(i,j-2)  ;
				qr_aux.at<uchar>(i,j-1)   = qr_in.at<uchar>(i,j-1)  ;
				qr_aux.at<uchar>(i,j)     = qr_in.at<uchar>(i,j)    ;
				qr_aux.at<uchar>(i,j+1)   = qr_in.at<uchar>(i,j+1)  ;
				qr_aux.at<uchar>(i,j+2)   = qr_in.at<uchar>(i,j+2)  ;

				qr_aux.at<uchar>(i+1,j-2) = qr_in.at<uchar>(i+1,j-2);
				qr_aux.at<uchar>(i+1,j-1) = qr_in.at<uchar>(i+1,j-1);
				qr_aux.at<uchar>(i+1,j)   = qr_in.at<uchar>(i+1,j)  ;
				qr_aux.at<uchar>(i+1,j+1) = qr_in.at<uchar>(i+1,j+1);
				qr_aux.at<uchar>(i+1,j+2) = qr_in.at<uchar>(i+1,j+2);

				qr_aux.at<uchar>(i+2,j-2) = qr_in.at<uchar>(i+2,j-2);
				qr_aux.at<uchar>(i+2,j-1) = qr_in.at<uchar>(i+2,j-1);
				qr_aux.at<uchar>(i+2,j)   = qr_in.at<uchar>(i+2,j)  ;
				qr_aux.at<uchar>(i+2,j+1) = qr_in.at<uchar>(i+2,j+1);
				qr_aux.at<uchar>(i+2,j+2) = qr_in.at<uchar>(i+2,j+2);
			}
		}
	}
	cv::resize(qr_in, qr_aux2, cv::Size(rows * 3,cols * 3),0,0, cv::INTER_NEAREST);
	cv::resize(qr_aux, qr_aux, cv::Size(rows * 3,cols * 3),0,0, cv::INTER_NEAREST);
	rows = rows * 3;
	cols = cols * 3;
	for (int i = 1; i <= rows-2; i+=3) {
		for (int j = 1; j <= cols-2; j+=3) {
			qr_aux.at<uchar>(i, j) = qr_aux2.at<uchar>(i, j);
		}
	}
    
	cv::resize(qr_aux, qr_aux, cv::Size(512,512),0,0, cv::INTER_NEAREST);
	qr_out = qr_aux.clone();
}

void importanceMap (Mat &outImg) /* Funcao para conservar somente a porcao fundamental da imagem a ser embutida. */
{
	Mat auxImg = outImg.clone();
    Canny(auxImg, auxImg, PARAMETROCANNY, PARAMETROCANNY);
	halftone(outImg);

	int linha_esq = 0, linha_dir = 0, linha_cima = 0, linha_baixo = 0;

	for (int i = 0; i < outImg.cols; i++)
		for (int j = 0; j < outImg.rows; j++)
			if (auxImg.at<uchar>(j, i) == 255)
			{
				linha_esq = i;
				i = outImg.cols; /* Forca interrupcao do laco mais externo. */
				break;
			}

	for (int i = outImg.cols-1; i >= 0; i--)
		for (int j = 0; j < outImg.rows; j++)
			if (auxImg.at<uchar>(j, i) == 255)
			{
				linha_dir = i;
				i = -1; /* Forca interrupcao do laco mais externo. */
				break;
			}

	for (int i = 0; i < outImg.rows; i++)
		for (int j = 0; j < outImg.cols; j++)
			if (auxImg.at<uchar>(i, j) == 255)
			{
				linha_cima = i;
				i = outImg.rows; /* Forca interrupcao do laco mais externo. */
				break;
			}


	for (int i = outImg.rows-1; i >= 0; i--)
		for (int j = 0; j < outImg.cols; j++)
			if (auxImg.at<uchar>(i, j) == 255)
			{
				linha_baixo = i;
				i = -1; /* Forca interrupcao do laco mais externo. */
				break;
			}

	int **posicoes_arestas;
	posicoes_arestas = (int**)calloc(auxImg.cols,sizeof(int*));
	for (int i = 0; i < auxImg.cols; i++)
		posicoes_arestas[i] = (int*)calloc(2,sizeof(int));

	int indice_colunas = 0;

	for (int i = 0; i < auxImg.cols; i++)
		for (int j = 0; j < 2; j++)
			posicoes_arestas[i][j] = 0;

	for (int i = 0; i < auxImg.cols; i++) // Preenche a ultima linha com pixels brancos para facilitar eliminacao de ruido. 
		auxImg.at<uchar>(auxImg.rows-1,i) = 255;

	
	for (int i = 0; i < auxImg.cols; i++)	/* Iteracao para obter o inicio e o fim do nucleo da imagem em cada coluna. */
	{
		for (int j = 0; j < auxImg.rows; j++)
			if (auxImg.at<uchar>(j,i) == 255)
				posicoes_arestas[i][0] = j;

		for (int k = auxImg.rows-1; k >= 0; k--)
			if (auxImg.at<uchar>(k,i) == 255)
				posicoes_arestas[i][1] = k;
	}

	halftone(outImg);

	auxImg = outImg.clone();

	for (int i = 0; i < auxImg.cols; i++)	// Iteracao para remover fundo da imagem 
		for (int j = 0; j < auxImg.rows; j++)
			if (j > posicoes_arestas[i][0] || j < posicoes_arestas[i][1])
				auxImg.at<uchar>(j,i) = BACKGROUND_COLOUR; // fundo cinza
                
	for (int i = 0; i < auxImg.cols; i++)
		free(posicoes_arestas[i]);
	free(posicoes_arestas);

	outImg = auxImg;
}

void mergeImg(Mat qr_in, Mat img, Mat qr_clear, Mat &out) // junta as 3 imagens recebidas em uma
{
	Mat aux(qr_in.size(), qr_in.type());
	int rows = qr_in.rows;
	int cols = qr_in.cols;

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			aux.at<uchar>(i, j) = (qr_clear.at<uchar>(i, j) == 205) ? img.at<uchar>(i, j) : qr_clear.at<uchar>(i, j);
			aux.at<uchar>(i, j) = (aux.at<uchar>(i, j) == 205) ? qr_in.at<uchar>(i, j) : aux.at<uchar>(i, j);
		}
	}

	out = aux.clone();
}

void monta_qrcode(Mat qr_original, Mat img, Mat &out) // monta qrcode com a imagem recebida
{
	Mat qr_resize = qr_original.clone();
	cv::resize(qr_resize, qr_resize, cv::Size(512,512),0,0, cv::INTER_NEAREST);
	Mat qr_clear;
	clear_qrcode(qr_original, qr_clear);
	Mat qr_merged(qr_resize.size(), qr_resize.type());

	importanceMap(img);

	cv::resize(img, img, cv::Size(512,512),0,0, cv::INTER_NEAREST);

	mergeImg(qr_resize, img, qr_clear, qr_merged);
    out = qr_merged.clone();
	imshow("QR Code", out);
}

int openFile(HWND hWnd, Mat &img, Mat &imgGray)
{
    char filename[FILENAME_MAX];
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(OPENFILENAME));

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = filename;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = FILENAME_MAX * 2;
    ofn.lpstrFilter = "Image Files (.jpg, .png, .bmp)\0*.JPG;*.PNG*;.BMP\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_FILEMUSTEXIST;

    GetOpenFileName(&ofn);

    if (!filename[0]) {
        return 0;
    }
    
    FILE *file;
    if (!(file = fopen(filename, "r"))) {
        MessageBox(NULL, "File not found!", "ERROR", MB_OK);
        return -1;
    }
    fclose(file);

    img = imread(filename);
    cvtColor(img, imgGray, COLOR_BGR2GRAY);
 
    cv::imshow("SRC", img);
    return 1;
}

int saveFile(HWND hWnd, Mat qrCode)
{
    char filename[FILENAME_MAX];
    OPENFILENAME sfn;

    ZeroMemory(&sfn, sizeof(OPENFILENAME));

    sfn.lStructSize = sizeof(OPENFILENAME);
    sfn.hwndOwner = hWnd;
    sfn.lpstrFile = filename;
    sfn.lpstrFile[0] = '\0';
    sfn.nMaxFile = FILENAME_MAX * 2;
    sfn.lpstrFilter = "PNG Image\0*.PNG\0JPEG Image\0*.JPG\0BMP Image\0*.BMP\0";
    sfn.nFilterIndex = 1;
    sfn.lpstrDefExt = "PNG\0JPG\0BMP\0";
    sfn.Flags = OFN_OVERWRITEPROMPT;

    GetSaveFileName(&sfn);

    if (!filename[0]) {
        return 0;
    }
    imwrite(filename, qrCode);
    return 1;
}

void generateHTQR(const Mat imgGray, const char *string, Mat &qrCodeImg) // gerar halftone qrcode
{
    QRcode *qrcode;
	qrcode = QRcode_encodeString(string, 0, QR_ECLEVEL_H, QR_MODE_8, 1);

	cv::Mat qr_original(qrcode->width, qrcode->width, CV_8UC1); 

	for (int i = 0; i < qrcode->width; i++) {
		for (int j = 0; j < qrcode->width; j++) {
			qr_original.at<uchar>(i, j) = (qrcode->data[i*qrcode->width + j] & 0x01) == 0x01 ? 0 : 255;
		}
	}

    monta_qrcode(qr_original, imgGray, qrCodeImg);
}

void generateQR(const char *string, Mat &qrCodeImg) // gerar qrcode normal
{
    QRcode *qrcode;
	qrcode = QRcode_encodeString(string, 0, QR_ECLEVEL_H, QR_MODE_8, 1);

	cv::Mat qr_original(qrcode->width, qrcode->width, CV_8UC1); 

	for (int i = 0; i < qrcode->width; i++) {
		for (int j = 0; j < qrcode->width; j++) {
			qr_original.at<uchar>(i, j) = (qrcode->data[i*qrcode->width + j] & 0x01) == 0x01 ? 0 : 255;
		}
	}

    cv::resize(qr_original, qrCodeImg, cv::Size(512,512),0,0, cv::INTER_NEAREST);
    imshow("QR Code", qrCodeImg);
}

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) // procs para definir o que os botoes fazem
{
    static unsigned char ini = 0;
    static unsigned char qr_ini = 0;
    static Mat img, imgGray, imgHalftone;
    static Mat qrCodeImg;
    int val;
    switch (msg) {
    case WM_COMMAND:
        switch (wp) {
        case PROC_INFO_MENU: 
            MessageBox(NULL, "Andrei Pochmann Koenich\nPedro Company Beck\n\nFederal University of Rio Grande do Sul\nFundamentals of Image Processing - 2021/2", "About", MB_OK);
            break;
        case PROC_EXIT_MENU:
            val = MessageBoxW(NULL, L"Exit?", L"", MB_YESNO | MB_ICONEXCLAMATION);
            if (val == IDYES) {
                exit(0);
            }
            break;
        case PROC_OPEN_FILE:
            if (openFile(hWnd, img, imgGray)) {
                ini = 1;
                waitKey(0);
            }
            break;
        case PROC_IMG_HALFTONE:
            if (!ini) {
                MessageBox(NULL, "Load an image first!", "ERROR", MB_OK);
            } else {
                imgHalftone = imgGray.clone();
                halftone(imgHalftone);
                imshow("SRC HALFTONE", imgHalftone);
                waitKey(0);
            }
            break;
        case PROC_GENERATE_HTQR:
            if (!ini) {
                MessageBox(NULL, "Load an image first!", "ERROR", MB_OK);
            } else {
                char text[BUFFER_SIZE] = { 0 };
                GetWindowText(editBox, text, BUFFER_SIZE);
                if (text[0]) {
                    generateHTQR(imgGray, text, qrCodeImg);
                    qr_ini = 1;
                }
              	waitKey(0);
            }
            break;
        case PROC_GENERATE_QR: {
            char text[BUFFER_SIZE] = { 0 };
            GetWindowText(editBox, text, BUFFER_SIZE);
            if (text[0]) {
                generateQR(text, qrCodeImg);
                qr_ini = 1;
            }
            waitKey(0);
            break;
        }
        case PROC_SAVE_FILE:
            if (!qr_ini) {
                MessageBox(NULL, "Generate a QR Code first!", "ERROR", MB_OK);
            } else if (saveFile(hWnd, qrCodeImg)) {
                ini = 1;
                waitKey(0);
            }
            break;
        }
        break;
    case WM_DESTROY:
        exit(0);
        break;
    default:
        return DefWindowProcW(hWnd, msg, wp, lp);
    }
}

void AddControls(HWND hWnd)
{
    CreateWindowW(
        L"BUTTON",
        L"LOAD IMAGE",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        20,         // x position 
        20,         // y position 
        200,        // Button width
        25,        // Button height
        hWnd,     // Parent window
        (HMENU)PROC_OPEN_FILE, // procedure
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

    CreateWindowW(
         L"BUTTON", L"HALFTONE IMAGE",
         WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
         20, 60,
         200, 25,
         hWnd,
         (HMENU)PROC_IMG_HALFTONE,
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);

    CreateWindowW(
         L"STATIC", L"INSERT TEXT BELOW",
         WS_TABSTOP | WS_VISIBLE | WS_CHILD,
         20, 100,
         180, 25,
         hWnd,
         NULL,
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);

    editBox = CreateWindowW(
         L"EDIT", L"Type your text here...",
         WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL,
         20, 120,
         200, 100,
         hWnd,
         NULL,
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);

    CreateWindowW(
         L"BUTTON", L"HALFTONE QR CODE",
         WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
         20, 240,
         200, 25,
         hWnd,
         (HMENU)PROC_GENERATE_HTQR,
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);

    CreateWindowW(
         L"BUTTON", L"NORMAL QR CODE",
         WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
         20, 280,
         200, 25,
         hWnd,
         (HMENU)PROC_GENERATE_QR,
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);

    CreateWindowW(
         L"BUTTON", L"SAVE QR CODE",
         WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
         20, 320,
         200, 25,
         hWnd,
         (HMENU)PROC_SAVE_FILE,
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);
}

void AddMenus(HWND hWnd, HMENU hMenu) 
{
    hMenu = CreateMenu();
    HMENU hFileMenu = CreateMenu();

    LPCSTR exit = "Exit";
    LPCSTR file = "File";
    LPCSTR info = "About";

    AppendMenu(hFileMenu, MF_STRING, PROC_EXIT_MENU, exit);

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, file);
    AppendMenu(hMenu, MF_STRING, PROC_INFO_MENU, info);
    
    SetMenu(hWnd, hMenu);
}

int main(int argc,char* argv[])
{	
	// hide cmd
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    WNDCLASSW wc = { 0 }; // define window class
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProcedure;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = (HINSTANCE)GetModuleHandle(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"WindowClass";

    // register class
    if (!RegisterClassW(&wc)) {
        return 0;
    }

    // create main window
    HWND hWnd = CreateWindowW(L"WindowClass", L"Halftone QR Codes",
                              WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              100, 100, 
                              260, 420,
                              NULL, NULL, NULL, NULL);
    if (!hWnd) {
        ShowWindow(GetConsoleWindow(), SW_SHOW);
        return 0;
    }

    // create menu and controls
    HMENU hMenu = { 0 };
    AddMenus(hWnd, hMenu);
    AddControls(hWnd);

    MSG msg = { 0 };

    UpdateWindow(hWnd);
    while (GetMessage(&msg, hWnd, 0, 0) != -1) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ShowWindow(GetConsoleWindow(), SW_SHOW);
	return 0;
}
