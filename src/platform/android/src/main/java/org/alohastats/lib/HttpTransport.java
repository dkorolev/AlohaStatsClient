/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2014 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *******************************************************************************/

package org.alohastats.lib;

import android.os.Build;
import android.util.Log;

import org.apache.http.util.ByteArrayBuffer;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;

public class HttpTransport {

  // TODO: optimize if needed for bigger transfers from the server
  private final static int REPLY_BUFFER_SIZE = 128;
  private final static String TAG = "HttpTransport";

  public static boolean postToDefaultUrl(final byte[] postBody)
  {
    return postToUrl(BuildConfig.STATISTICS_URL, postBody);
  }

  /**
   * Synchronous blocking call to HTTP server, optimized for smaller (< 1M) data transfers.
   */
  public static boolean postToUrl(final String httpUrl, final byte[] postBody)
  {
    HttpURLConnection urlConnection = null;
    final byte[] replyBuffer = new byte[REPLY_BUFFER_SIZE];
    final ByteArrayBuffer fullReplyBody = new ByteArrayBuffer(REPLY_BUFFER_SIZE);

    try {
      // Log.d(TAG, "Connecting to " + httpUrl);
      urlConnection = (HttpURLConnection) new URL(httpUrl).openConnection();
      urlConnection.setDoOutput(true);
      urlConnection.getOutputStream().write(postBody);

      int readBytesCount;
      do {
        readBytesCount = urlConnection.getInputStream().read(replyBuffer);
        if (readBytesCount > 0)
          fullReplyBody.append(replyBuffer, 0, replyBuffer.length);
      } while (readBytesCount > 0);

      final int httpCode = urlConnection.getResponseCode();
      //Log.d(TAG, "Server replied with http code " + httpCode);
      return 200 == httpCode;
    }
    catch (MalformedURLException e) {
      Log.e(TAG, e.getMessage());
    }
    catch (IOException e) {
      Log.e(TAG, e.getMessage());
    }
    finally {
      if (urlConnection != null)
        urlConnection.disconnect();
    }
    return false;
  }
}
