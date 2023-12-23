package com.example.nativecr

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.TextView
import com.example.nativecr.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

  private lateinit var binding: ActivityMainBinding

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)

    binding = ActivityMainBinding.inflate(layoutInflater)
    setContentView(binding.root)

    // Example of a call to a native method
    binding.sampleText.text = stringFromJNI() + "\n" + detectFrida() + "\n" + runpmlist("com.albarakaapp1", true)
  }

  /**
   * A native method that is implemented by the 'nativecr' native library,
   * which is packaged with this application.
   */
  external fun stringFromJNI(): String

  external fun detectFrida(): String

  external fun antiDebug(): String

  external fun runpmlist(packageName: String, exactMatch: Boolean): Boolean

  companion object {
    // Used to load the 'nativecr' library on application startup.
    init {
      System.loadLibrary("nativecr")
    }
  }
}