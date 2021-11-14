/*
  该程序访问主机的mjpg_streamer提供的Web访问接口，获取摄像头的图片。
  通过比较，判断是否有活动产生。
  比较的算法为：两个图像文件的大小差别在1%以上。
  比如：图像1的文件大小为48KB，图像2的文件大小为49KB，文件大小差别为1/48 = 2%，所以存在活动。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <curl/curl.h>

#define	URL		"http://%s:8080/?action=snapshot"
#define	FILENAME1	"%s/Activities/%s/%s/%s_a.jpg"
#define	FILENAME2	"%s/Activities/%s/%s/%s_b.jpg"

struct memory {
   char *data;
   time_t time;
   size_t size;
};

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
  struct memory *m = (struct memory *)userp;
  m->data = realloc (m->data, m->size + size * nmemb);
  memcpy (m->data + m->size, buffer, size * nmemb);
  m->size += size * nmemb;
  return size * nmemb;
}

void mkfiledir(char *filename)
{
  for(int i = 0; i < strlen(filename); i++)
  {
    if (filename[i] == '/')
    {
      filename[i] = '\0';
      if (stat(filename, NULL) == -1)
        mkdir(filename, 0755);
      filename[i] = '/';
    }
  }
}

int main(int argc, char *argv[])
{

  if (argc != 2)
  {
    fprintf (stderr, "Usage: %s <host>\n", argv[0]);
    exit (-1);
  }

  char host[32];
  sprintf (host, "%s", argv[1]);
  
  char home[4096];
  sprintf (home, "%s", getenv("HOME"));

  char url[64];
  sprintf (url, URL, argv[1]);

  CURL *handle;
  handle = curl_easy_init();
  if (!handle)
  {
    fprintf (stderr, "error: curl_easy_init()\n");
    exit (-1);
  }

  curl_easy_setopt(handle, CURLOPT_URL, url);
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
  curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 5L);
  curl_easy_setopt(handle, CURLOPT_TIMEOUT, 10L);

  CURLcode ret;
  char buf[CURL_ERROR_SIZE];
  curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, &buf);

  struct memory image1 = {0};
  time (&image1.time);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)&image1);

  ret = curl_easy_perform(handle);
  if (ret)
  {
    fprintf (stderr, "error: curl_easy_perform()\n");
    fprintf (stderr, "msg: %s\n", buf);
    exit (-1);
  }

  sleep(30);
  struct memory image2 = {0};
  time (&image2.time);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)&image2);
  ret = curl_easy_perform(handle);
  if (ret)
  {
    fprintf (stderr, "error: curl_easy_perform()\n");
    fprintf (stderr, "msg: %s\n", buf);
    exit (-1);
  }

  /*
    判断大小
  */
  unsigned int diff = abs((int)image1.size - (int)image2.size);
  unsigned int avg = (image1.size + image2.size) / 2;
  if (diff * 100 > avg)
  {
    FILE *fp;
    char filename[4096];
    char date[32];
    char time[32];
    struct tm *t;

    t = localtime(&image1.time);

    sprintf (date, "%02d%02d%02d" , t->tm_year, t->tm_mon + 1, t->tm_mday);
    sprintf (time, "%02d%02d%02d" , t->tm_hour, t->tm_min, t->tm_sec);
    sprintf (filename, FILENAME1, home, host, date, time);

    mkfiledir(filename);
    fp = fopen (filename, "wb");
    if (!fp)
    {
      fprintf (stderr, "Error: fopen()\n");
      fprintf (stderr, "msg: can't open file: %s\n", filename);
      exit (-1);
    }

    fwrite (image1.data, 1, image1.size, fp);
    fclose (fp);

    t = localtime(&image2.time);

    sprintf (filename, FILENAME2, home, host, date, time);
    mkfiledir(filename);

    fp = fopen (filename, "wb");
    if (!fp)
    {
      fprintf (stderr, "Error: fopen()\n");
      fprintf (stderr, "msg: can't open file: %s\n", filename);
      exit (-1);
    }
    fwrite (image2.data, 1, image2.size, fp);
    fclose (fp);
  }

  free (image1.data);
  free (image2.data);
  curl_easy_cleanup(handle);
  return 0;
}
