/*
   Copyright 2023 Trail of Bits

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

package com.android.covertVKWriter

import android.content.res.AssetManager
import android.graphics.Color
import android.os.Bundle
import android.widget.Button
import android.widget.TextView
import android.widget.EditText
import android.os.Handler
import androidx.appcompat.app.AppCompatActivity

class CovertVKWriter : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main);
        val writeMemoryButton = findViewById<Button>(R.id.writeMemoryButton)
        val statusTextView = findViewById<TextView>(R.id.statusTextView)
        val itersEditText = findViewById<TextView>(R.id.editTextIterations)
        val canaryEditText = findViewById<TextView>(R.id.editTextCanaryValue)

        writeMemoryButton.setOnClickListener {

            val assetManager = this.assets;
            val handler = Handler()

            statusTextView.setText("Starting to write")

            handler.post {
                writeMemoryButton.isEnabled = false
                val iters: Int = itersEditText.text.toString().toInt();
                val canary: Int = canaryEditText.text.toString().toInt();

                for (i in 0..iters - 1) {
                    val k = this.entry(assetManager, canary);
                }

                statusTextView.setText("Done Writing")
                writeMemoryButton.isEnabled = true
            }
        }
    }

    // external fun entry(): IntArray
   external fun entry(assetManager : AssetManager, canary: Int): IntArray
    companion object {
        init {
            System.loadLibrary("vkwriter")
        }
    }
}