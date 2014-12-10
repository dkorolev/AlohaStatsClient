package org.alohastats.test;

import android.test.InstrumentationTestCase;

import org.alohastats.lib.HttpTransport;

/**
 * <a href="http://d.android.com/tools/testing/testing_android.html">Testing Fundamentals</a>
 */
public class HttpTransportTest extends InstrumentationTestCase
{
  public void testHttpPost() throws Exception {
    final String postBody = "Hello, World!";
    assertTrue(HttpTransport.postToUrl("http://httpbin.org/post", postBody.getBytes()));
  }
}
