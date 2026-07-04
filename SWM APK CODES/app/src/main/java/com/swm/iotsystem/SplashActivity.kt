package com.swm.iotsystem

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.View
import android.view.animation.DecelerateInterpolator
import android.widget.ProgressBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity

class SplashActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_splash)

        val tvSWM = findViewById<TextView>(R.id.tvSWM)
        val dividerLine = findViewById<View>(R.id.dividerLine)
        val tvSubtitle = findViewById<TextView>(R.id.tvSubtitle)
        val tvTagline = findViewById<TextView>(R.id.tvTagline)
        val progressBar = findViewById<ProgressBar>(R.id.progressBar)
        val tvVersion = findViewById<TextView>(R.id.tvVersion)

        // Initially hide all elements for animation
        tvSWM.alpha = 0f
        tvSWM.scaleX = 0.7f
        tvSWM.scaleY = 0.7f
        dividerLine.alpha = 0f
        dividerLine.scaleX = 0f
        tvSubtitle.alpha = 0f
        tvSubtitle.translationY = 40f
        tvTagline.alpha = 0f
        progressBar.alpha = 0f
        tvVersion.alpha = 0f

        // Animate SWM text — scale up with glow effect
        tvSWM.animate()
            .alpha(1f).scaleX(1f).scaleY(1f)
            .setDuration(900)
            .setInterpolator(DecelerateInterpolator(2f))
            .start()

        // Animate divider line — expand from center
        dividerLine.animate()
            .alpha(1f).scaleX(1f)
            .setDuration(600)
            .setStartDelay(450)
            .setInterpolator(DecelerateInterpolator())
            .start()

        // Animate IOT SYSTEM — slide up and fade in
        tvSubtitle.animate()
            .alpha(1f).translationY(0f)
            .setDuration(650)
            .setStartDelay(650)
            .setInterpolator(DecelerateInterpolator(1.5f))
            .start()

        // Animate FUTURISTIC TECHNOLOGY tagline
        tvTagline.animate()
            .alpha(1f)
            .setDuration(500)
            .setStartDelay(1000)
            .start()

        // Animate loading bar
        progressBar.animate()
            .alpha(1f)
            .setDuration(400)
            .setStartDelay(1100)
            .start()

        // Animate version text
        tvVersion.animate()
            .alpha(1f)
            .setDuration(400)
            .setStartDelay(1200)
            .start()

        // Navigate after animations complete
        Handler(Looper.getMainLooper()).postDelayed({
            val prefs = getSharedPreferences("smart_fridge_prefs", Context.MODE_PRIVATE)
            val host = prefs.getString("mqtt_host", "")

            val intent = if (!host.isNullOrEmpty()) {
                Intent(this, DashboardActivity::class.java)
            } else {
                Intent(this, ConfigActivity::class.java)
            }
            startActivity(intent)
            @Suppress("DEPRECATION")
            overridePendingTransition(R.anim.fade_in, R.anim.fade_out)
            finish()
        }, 2800)
    }
}
