package com.swm.iotsystem

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.view.animation.DecelerateInterpolator
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.hivemq.client.mqtt.MqttClient
import java.nio.charset.StandardCharsets

class ConfigActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_config)

        val etHost = findViewById<EditText>(R.id.etHost)
        val etUser = findViewById<EditText>(R.id.etUser)
        val etPass = findViewById<EditText>(R.id.etPass)
        val btnSave = findViewById<Button>(R.id.btnSave)

        // Staggered Entrance Animations
        val views = listOf(etHost, etUser, etPass, btnSave)
        views.forEachIndexed { index, view ->
            view.alpha = 0f
            view.translationY = 50f
            view.animate()
                .alpha(1f)
                .translationY(0f)
                .setDuration(500)
                .setStartDelay((index * 100).toLong())
                .setInterpolator(DecelerateInterpolator(1.5f))
                .start()
        }

        btnSave.setOnClickListener {
            var host = etHost.text.toString().trim()
            val user = etUser.text.toString().trim()
            val pass = etPass.text.toString().trim()

            if (host.isEmpty() || user.isEmpty() || pass.isEmpty()) {
                Toast.makeText(this, "PLEASE FILL ALL!", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            if (host.startsWith("tcp://")) {
                host = host.replace("tcp://", "")
            }

            btnSave.isEnabled = false
            btnSave.text = "CONNECTING..."

            try {
                val builder = MqttClient.builder()
                    .useMqttVersion3()
                    .identifier("TestClient_" + System.currentTimeMillis())
                    .serverHost(host)
                    .serverPort(8883)
                    .sslWithDefaultConfig()

                if (user.isNotEmpty()) {
                    builder.simpleAuth()
                        .username(user)
                        .password(pass.toByteArray(StandardCharsets.UTF_8))
                        .applySimpleAuth()
                }

                val testClient = builder.buildAsync()

                testClient.connect().whenComplete { _, throwable ->
                    runOnUiThread {
                        btnSave.isEnabled = true
                        btnSave.text = "CONNECT & SAVE"

                        if (throwable != null) {
                            Log.e("MQTT", "Failed to connect", throwable)
                            Toast.makeText(this, "Connection Failed! Wrong details.", Toast.LENGTH_LONG).show()
                        } else {
                            // Connection successful
                            testClient.disconnect()

                            val prefs = getSharedPreferences("smart_fridge_prefs", Context.MODE_PRIVATE)
                            with(prefs.edit()) {
                                putString("mqtt_host", host)
                                putString("mqtt_user", user)
                                putString("mqtt_pass", pass)
                                apply()
                            }

                            startActivity(Intent(this, DashboardActivity::class.java))
                            @Suppress("DEPRECATION")
                            overridePendingTransition(R.anim.slide_in_right, R.anim.slide_out_left)
                            finish()
                        }
                    }
                }
            } catch (e: Exception) {
                e.printStackTrace()
                btnSave.isEnabled = true
                btnSave.text = "CONNECT & SAVE"
                Toast.makeText(this, "Invalid Host Address!", Toast.LENGTH_LONG).show()
            }
        }
    }
}
