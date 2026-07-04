package com.swm.iotsystem

import android.content.Context
import android.content.Intent
import android.graphics.Color
import android.net.ConnectivityManager
import android.net.Network
import android.net.NetworkCapabilities
import android.net.NetworkRequest
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.Menu
import android.view.MenuItem
import android.view.View
import android.view.ViewGroup
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.LinearLayout
import android.widget.SeekBar
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import android.widget.Button
import android.annotation.SuppressLint
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.Toolbar
import androidx.cardview.widget.CardView
import com.airbnb.lottie.LottieAnimationView
import com.airbnb.lottie.LottieDrawable
import com.google.android.material.switchmaterial.SwitchMaterial
import com.hivemq.client.mqtt.MqttClient
import com.hivemq.client.mqtt.mqtt3.Mqtt3AsyncClient
import com.google.android.material.bottomsheet.BottomSheetDialog
import android.net.Uri
import android.view.animation.DecelerateInterpolator
import android.animation.ArgbEvaluator
import android.text.Spannable
import android.text.SpannableString
import android.text.style.ForegroundColorSpan
import java.nio.charset.StandardCharsets

@SuppressLint("SetTextI18n", "InflateParams", "SpellCheckingInspection")
class DashboardActivity : AppCompatActivity() {

    private var currentWaterTemp = 0.0
    private var maxHotLimit = 40
    private var minColdLimit = 15
    private var mqttClient: Mqtt3AsyncClient? = null

    // Force Temperature Mode
    private var isForceMode = false
    private var forceTemp = 25
    private var tempUnit = 0 // 0: C, 1: F, 2: K
    private var isEmergency = false
    private var currentOutsideTemp = 0.0

    private lateinit var tvStatusBanner: TextView
    private lateinit var tvWaterTemp: TextView
    private lateinit var tvHelloUser: TextView
    private lateinit var tvEspHotLimit: TextView
    private lateinit var tvEspColdLimit: TextView
    private lateinit var cardHot: CardView
    private lateinit var cardCold: CardView
    private lateinit var lottieEmoji: LottieAnimationView
    private lateinit var lottieBg: LottieAnimationView

    // Force Mode UI references
    private lateinit var cardForce: CardView
    private lateinit var switchForce: SwitchMaterial
    private lateinit var tvForceTemp: TextView
    private lateinit var layoutForceSeekbar: LinearLayout
    private lateinit var sbForce: SeekBar
    private lateinit var tvForceInfo: TextView

    private val handler = Handler(Looper.getMainLooper())
    private var lastEsp32SignalTime: Long = 0
    private val heartbeatRunnable = object : Runnable {
        override fun run() {
            if (mqttClient?.state?.isConnected == true) {
                val timeSinceLastSignal = System.currentTimeMillis() - lastEsp32SignalTime
                if (timeSinceLastSignal > 15000) { // 15 seconds timeout
                    showBanner("SWM OFF - NO ESP32 SIGNAL", "#FF9800")
                } else {
                    hideBanner()
                }
            }
            handler.postDelayed(this, 5000) // Check every 5 seconds
        }
    }

    // Safe public Lottie URLs for demo
    private val urlFireBg = "https://lottie.host/9e7c53d1-9489-4e78-9041-e970a2569578/B5q4K2fE89.json" 
    private val urlIceBg = "https://lottie.host/43a6f1b0-9db4-46c5-8408-726d40ad2fbc/Y98M6i0RkK.json"
    
    private val urlEmojiHot = "https://lottie.host/d1933a2c-f604-4537-881b-873b22ed7c31/z0L26R1sLh.json"
    private val urlEmojiCold = "https://lottie.host/02251a37-567a-40a2-ad3b-7489508bc17b/a2eQ3c6M2g.json"
    private val urlEmojiNormal = "https://lottie.host/fbcf9a3e-1082-4414-b6c8-5089fde5a8b0/G3n9N8Pq9d.json"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_dashboard)

        tvStatusBanner = findViewById(R.id.tvStatusBanner)
        cardHot = findViewById(R.id.cardHot)
        cardCold = findViewById(R.id.cardCold)
        lottieEmoji = findViewById(R.id.lottieEmoji)
        lottieBg = findViewById(R.id.lottieBg)

        // Force Mode UI
        cardForce = findViewById(R.id.cardForce)
        switchForce = findViewById(R.id.switchForce)
        tvForceTemp = findViewById(R.id.tvForceTemp)
        layoutForceSeekbar = findViewById(R.id.layoutForceSeekbar)
        sbForce = findViewById(R.id.sbForce)
        tvForceInfo = findViewById(R.id.tvForceInfo)

        // Prevent crash if Lottie URL fails (e.g. 403 Forbidden)
        lottieEmoji.setFailureListener { e -> Log.e("Lottie", "Emoji Load Failed", e) }
        lottieBg.setFailureListener { e -> Log.e("Lottie", "Bg Load Failed", e) }

        val toolbar = findViewById<Toolbar>(R.id.toolbar)
        setSupportActionBar(toolbar)
        supportActionBar?.title = "SWM IOT SYSTEM"
        supportActionBar?.subtitle = "GPP7 EC 2025-28"
        toolbar.setTitleTextColor(Color.WHITE)
        toolbar.setSubtitleTextColor(Color.parseColor("#7B8CA8"))

        tvHelloUser = findViewById(R.id.tvHelloUser)
        val prefs = getSharedPreferences("smart_fridge_prefs", Context.MODE_PRIVATE)
        val savedName = prefs.getString("user_name", "User") ?: "User"
        updateHelloUserText(savedName)

        val etZipCode = findViewById<EditText>(R.id.etZipCode)

        // Restore previously saved zip code
        val savedZipCode = getSharedPreferences("smart_fridge_prefs", Context.MODE_PRIVATE)
            .getString("selected_zip_code", "110001") ?: "110001"
        etZipCode.setText(savedZipCode)

        tvWaterTemp = findViewById(R.id.tvWaterTemp)
        tvWaterTemp.text = formatTemp(currentWaterTemp)

        tvEspHotLimit = findViewById(R.id.tvEspHotLimit)
        tvEspColdLimit = findViewById(R.id.tvEspColdLimit)
        
        val btnUnitToggle = findViewById<LinearLayout>(R.id.btnUnitToggle)
        val tvCurrentUnit = findViewById<TextView>(R.id.tvCurrentUnit)
        btnUnitToggle.setOnClickListener {
            tempUnit = (tempUnit + 1) % 3
            val units = arrayOf("°C", "°F", "K")
            tvCurrentUnit.text = units[tempUnit]
            updateTempDisplays()
        }
        
        val btnEmergency = findViewById<Button>(R.id.btnEmergency)
        btnEmergency.setOnClickListener {
            isEmergency = !isEmergency
            if (isEmergency) {
                btnEmergency.setBackgroundResource(R.drawable.bg_cyberpunk_solid_red_button)
                btnEmergency.setTextColor(Color.WHITE)
                publishMessage("fridge/emergency", "ON")
                showCustomToast("EMERGENCY SHUTDOWN ACTIVATED ON SWM!")
            } else {
                btnEmergency.setBackgroundResource(R.drawable.bg_cyberpunk_glass_red_button)
                btnEmergency.setTextColor(Color.parseColor("#FF5252"))
                publishMessage("fridge/emergency", "OFF")
                showCustomToast("SWM RESTORED TO NORMAL")
            }
        }

        val btnSaveZip = findViewById<Button>(R.id.btnSaveZip)
        btnSaveZip.setOnClickListener {
            val zipText = etZipCode.text.toString().trim()
            if (zipText.length != 6) {
                showCustomToast("PLEASE ENTER A VALID 6-DIGIT PIN CODE")
                return@setOnClickListener
            }
            
            getSharedPreferences("smart_fridge_prefs", Context.MODE_PRIVATE)
                .edit().putString("selected_zip_code", zipText).apply()
                
            publishMessage("fridge/setZip", zipText)
            showCustomToast("VALIDATING NEW ZIP CODE ON SWM...")
        }

        val btnUpdateData = findViewById<Button>(R.id.btnUpdateData)
        btnUpdateData.setOnClickListener {
            // Only update temperature limits, not the ZIP code!
            publishMessage("fridge/setHotLimit", maxHotLimit.toString())
            publishMessage("fridge/setColdLimit", minColdLimit.toString())
            
            // If force mode is active, resend that too
            if (isForceMode) {
                publishMessage("fridge/forceMode", "ON")
                publishMessage("fridge/forceTemp", forceTemp.toString())
            } else {
                publishMessage("fridge/forceMode", "OFF")
            }
            
            showCustomToast("UPDATING SWM LIMITS...")
        }

        val btnUpdate = findViewById<Button>(R.id.btnUpdate)
        btnUpdate.setOnClickListener {
            // ONLY request a sync, DO NOT push app changes
            publishMessage("fridge/requestSync", "SYNC")
            showCustomToast("REQUESTED DATA SYNC FROM SWM...")
        }

        val tvHotLimit = findViewById<TextView>(R.id.tvHotLimit)
        val sbHot = findViewById<SeekBar>(R.id.sbHot)
        sbHot.progress = maxHotLimit - 30
        updateHotCardBackground(maxHotLimit)
        
        sbHot.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                maxHotLimit = progress + 30
                tvHotLimit.text = "${maxHotLimit}°C"
                updateHotCardBackground(maxHotLimit)
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })

        val tvColdLimit = findViewById<TextView>(R.id.tvColdLimit)
        val sbCold = findViewById<SeekBar>(R.id.sbCold)
        sbCold.progress = minColdLimit - 5
        updateColdCardBackground(minColdLimit)
        
        sbCold.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                minColdLimit = progress + 5
                tvColdLimit.text = "${minColdLimit}°C"
                updateColdCardBackground(minColdLimit)
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })

        // ===== Force Temperature Mode Setup =====
        setupForceMode()

        updateEmojisAndBackground(currentWaterTemp)
        setupNetworkListener()
        setupMqtt()

        // ===== Staggered Entrance Animations =====
        val layoutRegion = findViewById<LinearLayout>(R.id.layoutRegion)
        val layoutTempRing = findViewById<LinearLayout>(R.id.layoutTempRing)
        val cards = listOf(layoutRegion, layoutTempRing, cardHot, cardCold, cardForce)
        
        cards.forEachIndexed { index, view ->
            view.alpha = 0f
            view.translationY = 80f
            view.animate()
                .alpha(1f)
                .translationY(0f)
                .setDuration(600)
                .setStartDelay((index * 150).toLong())
                .setInterpolator(DecelerateInterpolator(1.5f))
                .start()
        }
    }

    // ===== FORCE TEMPERATURE MODE =====

    private fun setupForceMode() {
        sbForce.progress = forceTemp - 5 // SeekBar 0–55 maps to 5–60°C
        tvForceTemp.text = "${forceTemp}°C"

        // Toggle force mode ON/OFF
        switchForce.setOnCheckedChangeListener { _, isChecked ->
            setForceMode(isChecked)
        }

        // Force temperature seekbar
        sbForce.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                forceTemp = progress + 5
                tvForceTemp.text = "${forceTemp}°C"
                updateForceCardColor(forceTemp)
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })
    }

    private fun setForceMode(enabled: Boolean) {
        isForceMode = enabled

        if (enabled) {
            // ── Blur & disable Hot/Cold cards with smooth animation ──
            cardHot.animate().alpha(0.25f).setDuration(350).start()
            cardCold.animate().alpha(0.25f).setDuration(350).start()
            setViewAndChildrenEnabled(cardHot, false)
            setViewAndChildrenEnabled(cardCold, false)

            // ── Glow up force card background ──
            cardForce.animate().setDuration(300).start()
            updateForceCardColor(forceTemp)

            // ── Reveal force controls with staggered animations ──
            tvForceTemp.visibility = View.VISIBLE
            layoutForceSeekbar.visibility = View.VISIBLE
            tvForceInfo.visibility = View.VISIBLE

            tvForceTemp.alpha = 0f
            tvForceTemp.translationY = 20f
            layoutForceSeekbar.alpha = 0f
            layoutForceSeekbar.translationY = 20f
            tvForceInfo.alpha = 0f

            tvForceTemp.animate().alpha(1f).translationY(0f).setDuration(350).start()
            layoutForceSeekbar.animate().alpha(1f).translationY(0f).setDuration(350).setStartDelay(120).start()
            tvForceInfo.animate().alpha(1f).setDuration(300).setStartDelay(250).start()

            // ── Publish to ESP32 ──
            publishMessage("fridge/forceMode", "ON")
            publishMessage("fridge/forceTemp", forceTemp.toString())

            showCustomToast("⚡ FORCE MODE ON — ${forceTemp}°C")

        } else {
            // ── Restore Hot/Cold cards with smooth animation ──
            cardHot.animate().alpha(1f).setDuration(350).start()
            cardCold.animate().alpha(1f).setDuration(350).start()
            setViewAndChildrenEnabled(cardHot, true)
            setViewAndChildrenEnabled(cardCold, true)

            // ── Dim force card background ──
            cardForce.setCardBackgroundColor(Color.argb(34, 255, 152, 0))

            // ── Hide force controls with fade-out ──
            tvForceTemp.animate().alpha(0f).translationY(20f).setDuration(250)
                .withEndAction { tvForceTemp.visibility = View.GONE }.start()
            layoutForceSeekbar.animate().alpha(0f).translationY(20f).setDuration(250)
                .withEndAction { layoutForceSeekbar.visibility = View.GONE }.start()
            tvForceInfo.animate().alpha(0f).setDuration(200)
                .withEndAction { tvForceInfo.visibility = View.GONE }.start()

            // ── Publish to ESP32 ──
            publishMessage("fridge/forceMode", "OFF")
            
            // BUG FIX: Re-send the actual boundaries so ESP32 knows them again
            publishMessage("fridge/setHotLimit", maxHotLimit.toString())
            publishMessage("fridge/setColdLimit", minColdLimit.toString())

            showCustomToast("FORCE MODE OFF — NORMAL BOUNDARIES RESTORED")
        }
    }

    /**
     * Recursively enable/disable all child views of a ViewGroup.
     * Used to make Hot/Cold cards non-interactive during Force Mode.
     */
    private fun setViewAndChildrenEnabled(view: View, enabled: Boolean) {
        view.isEnabled = enabled
        if (view is ViewGroup) {
            for (i in 0 until view.childCount) {
                setViewAndChildrenEnabled(view.getChildAt(i), enabled)
            }
        }
    }

    // ===== EXISTING METHODS =====
    
    private fun updateHelloUserText(name: String) {
        val text = "HELLO, ${name.uppercase()}! \uD83D\uDC4B"
        val spannable = SpannableString(text)
        val colors = intArrayOf(
            Color.parseColor("#FF5252"), // Red
            Color.parseColor("#40C4FF"), // Cyan
            Color.parseColor("#FFB300"), // Amber
            Color.parseColor("#69F0AE"), // Green
            Color.parseColor("#E040FB")  // Purple
        )
        var colorIndex = 0
        for (i in text.indices) {
            val c = text[i]
            if (c.isLetterOrDigit()) {
                spannable.setSpan(ForegroundColorSpan(colors[colorIndex % colors.size]), i, i + 1, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE)
                colorIndex++
            }
        }
        tvHelloUser.text = spannable
    }

    private fun formatTemp(celsius: Double, removeC: Boolean = false): String {
        val res = when (tempUnit) {
            1 -> String.format("%.1f°F", (celsius * 9/5) + 32)
            2 -> String.format("%.1fK", celsius + 273.15)
            else -> String.format("%.1f°C", celsius)
        }
        return if (removeC) res.replace("°C", "").replace("°F", "").replace("K", "") else res
    }
    
    private fun updateTempDisplays() {
        tvWaterTemp.text = formatTemp(currentWaterTemp)
        findViewById<TextView>(R.id.tvOutsideTemp).text = formatTemp(currentOutsideTemp, true)
        
        val unitStr = when (tempUnit) {
            1 -> "°F"
            2 -> "K"
            else -> "°C"
        }
        findViewById<TextView>(R.id.tvOutsideUnitLabel).text = "$unitStr Feels Like"
    }

    private fun updateForceCardColor(temp: Int) {
        if (!isForceMode) return
        val layoutForceContent = findViewById<LinearLayout>(R.id.layoutForceContent)
        
        val evaluator = ArgbEvaluator()
        val colorCold = Color.parseColor("#40C4FF")
        val colorNormal = Color.parseColor("#FFB300")
        val colorHot = Color.parseColor("#FF5252")
        
        val color: Int
        if (temp <= 25) {
            val fraction = (temp - 5) / 20f // 5C is 0, 25C is 1
            color = evaluator.evaluate(Math.max(0f, Math.min(1f, fraction)), colorCold, colorNormal) as Int
            layoutForceContent.setBackgroundResource(R.drawable.bg_smoke_cold)
        } else {
            val fraction = (temp - 25) / 35f // 25C is 0, 60C is 1
            color = evaluator.evaluate(Math.max(0f, Math.min(1f, fraction)), colorNormal, colorHot) as Int
            layoutForceContent.setBackgroundResource(R.drawable.bg_smoke_hot)
        }
        
        sbForce.progressTintList = android.content.res.ColorStateList.valueOf(color)
        sbForce.thumbTintList = android.content.res.ColorStateList.valueOf(color)
    }
    
    private fun updateHotCardBackground(temp: Int) {
        // Handled completely by Ultra HD XML (bg_smoke_hot.xml) now
        cardHot.setCardBackgroundColor(Color.TRANSPARENT)
    }

    private fun updateColdCardBackground(temp: Int) {
        // Handled completely by Ultra HD XML (bg_smoke_cold.xml) now
        cardCold.setCardBackgroundColor(Color.TRANSPARENT)
    }

    private fun updateEmojisAndBackground(temp: Double) {
        try {
            if (temp < 15.0) {
                lottieEmoji.setAnimationFromUrl(urlEmojiCold)
                lottieBg.setAnimationFromUrl(urlIceBg)
            } else if (temp > 30.0) {
                lottieEmoji.setAnimationFromUrl(urlEmojiHot)
                lottieBg.setAnimationFromUrl(urlFireBg)
            } else {
                lottieEmoji.setAnimationFromUrl(urlEmojiNormal)
                lottieBg.cancelAnimation()
                lottieBg.setImageDrawable(null) // clear background animation
            }
            lottieEmoji.repeatCount = LottieDrawable.INFINITE
            lottieBg.repeatCount = LottieDrawable.INFINITE
            lottieEmoji.playAnimation()
            lottieBg.playAnimation()
        } catch (e: Exception) {
            Log.e("Lottie", "Failed to load animation", e)
        }
    }

    private fun setupNetworkListener() {
        val connectivityManager = getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
        val networkRequest = NetworkRequest.Builder()
            .addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
            .build()
            
        connectivityManager.registerNetworkCallback(networkRequest, object : ConnectivityManager.NetworkCallback() {
            override fun onAvailable(network: Network) {
                runOnUiThread {
                    if (mqttClient?.state?.isConnected == false) {
                        showBanner("NO SWM DETECTED", "#FF9800")
                    } else {
                        hideBanner()
                    }
                }
            }
            override fun onLost(network: Network) {
                runOnUiThread {
                    showBanner("NO INTERNET", "#D32F2F")
                }
            }
        })
    }

    private fun showBanner(message: String, colorHex: String) {
        tvStatusBanner.text = message
        tvStatusBanner.setBackgroundColor(Color.parseColor(colorHex))
        tvStatusBanner.visibility = View.VISIBLE
    }

    private fun hideBanner() {
        tvStatusBanner.visibility = View.GONE
    }

    private fun setupMqtt() {
        val prefs = getSharedPreferences("smart_fridge_prefs", Context.MODE_PRIVATE)
        var host = prefs.getString("mqtt_host", "") ?: ""
        val user = prefs.getString("mqtt_user", "") ?: ""
        val pass = prefs.getString("mqtt_pass", "") ?: ""

        if (host.startsWith("tcp://")) {
            host = host.replace("tcp://", "")
        }

        try {
            val builder = MqttClient.builder()
                .useMqttVersion3()
                .identifier("AndroidAppClient_" + System.currentTimeMillis())
                .serverHost(if (host.isNotEmpty()) host else "localhost")
                .serverPort(8883)
                .sslWithDefaultConfig()
                
            if (user.isNotEmpty()) {
                builder.simpleAuth()
                    .username(user)
                    .password(pass.toByteArray(StandardCharsets.UTF_8))
                    .applySimpleAuth()
            }
            
            mqttClient = builder.buildAsync()

            mqttClient?.connect()?.whenComplete { _, throwable ->
                runOnUiThread {
                    if (throwable != null) {
                        showBanner("MQTT CONNECTION FAILED", "#D32F2F")
                    } else {
                        showCustomToast("CONNECTED TO SWM IOT SYSTEM!")
                        subscribeToTopics()
                        lastEsp32SignalTime = System.currentTimeMillis()
                        handler.post(heartbeatRunnable)
                    }
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
            runOnUiThread {
                showBanner("INVALID HOST", "#D32F2F")
            }
        }
    }

    private fun subscribeToTopics() {
        mqttClient?.let { client ->
            client.subscribeWith()
                .topicFilter("fridge/waterTemp")
                .callback { publish ->
                    val message = String(publish.payloadAsBytes, StandardCharsets.UTF_8)
                    runOnUiThread {
                        lastEsp32SignalTime = System.currentTimeMillis()
                        try {
                            val temp = message.toDouble()
                            currentWaterTemp = temp
                            updateTempDisplays()
                            updateEmojisAndBackground(temp)
                            hideBanner()
                        } catch (e: Exception) {
                            Log.e("MQTT", "Failed to parse temp: $message")
                        }
                    }
                }
                .send()

            client.subscribeWith()
                .topicFilter("fridge/feedback/hotLimit")
                .callback { publish ->
                    val message = String(publish.payloadAsBytes, StandardCharsets.UTF_8)
                    runOnUiThread {
                        val espHot = message.toIntOrNull()
                        if (espHot != null) {
                            tvEspHotLimit.text = "${espHot}°C"
                        }
                    }
                }
                .send()

            client.subscribeWith()
                .topicFilter("fridge/feedback/coldLimit")
                .callback { publish ->
                    val message = String(publish.payloadAsBytes, StandardCharsets.UTF_8)
                    runOnUiThread {
                        val espCold = message.toIntOrNull()
                        if (espCold != null) {
                            tvEspColdLimit.text = "${espCold}°C"
                        }
                    }
                }
                .send()
            client.subscribeWith()
                .topicFilter("fridge/feedback/zipStatus")
                .callback { publish ->
                    val message = String(publish.payloadAsBytes, StandardCharsets.UTF_8)
                    runOnUiThread {
                        if (message == "ERROR") {
                            showBanner("WRONG ZIP CODE", "#D32F2F")
                        } else if (message == "SUCCESS") {
                            hideBanner()
                        }
                    }
                }
                .send()
                
            client.subscribeWith()
                .topicFilter("fridge/signal")
                .callback { publish ->
                    val msg = String(publish.payloadAsBytes, StandardCharsets.UTF_8)
                    runOnUiThread { findViewById<TextView>(R.id.tvSignal).text = "SIGNAL: $msg%" }
                }.send()

            client.subscribeWith()
                .topicFilter("fridge/ping")
                .callback { publish ->
                    val msg = String(publish.payloadAsBytes, StandardCharsets.UTF_8)
                    runOnUiThread { findViewById<TextView>(R.id.tvPing).text = "PING: ${msg}ms" }
                }.send()
                
            client.subscribeWith()
                .topicFilter("fridge/weatherData")
                .callback { publish ->
                    val msg = String(publish.payloadAsBytes, StandardCharsets.UTF_8)
                    runOnUiThread { 
                        currentOutsideTemp = msg.toDoubleOrNull() ?: 0.0
                        updateTempDisplays()
                    }
                }.send()
                
            client.subscribeWith()
                .topicFilter("fridge/uptime")
                .callback { publish ->
                    val msg = String(publish.payloadAsBytes, StandardCharsets.UTF_8)
                    runOnUiThread { findViewById<TextView>(R.id.tvUptime).text = "UPTIME: $msg" }
                }.send()
        }
    }

    private fun publishMessage(topic: String, message: String) {
        mqttClient?.let { client ->
            if (client.state.isConnected) {
                client.publishWith()
                    .topic(topic)
                    .payload(message.toByteArray(StandardCharsets.UTF_8))
                    .send()
            } else {
                showCustomToast("NOT CONNECTED TO SWM")
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        handler.removeCallbacks(heartbeatRunnable)
        mqttClient?.disconnect()
    }

    override fun onCreateOptionsMenu(menu: Menu?): Boolean {
        menu?.add(0, 1, 0, "Settings")?.apply {
            setIcon(android.R.drawable.ic_menu_preferences)
            setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS)
        }
        return super.onCreateOptionsMenu(menu)
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        if (item.itemId == 1) {
            showSettingsDialog()
            return true
        }
        return super.onOptionsItemSelected(item)
    }

    private fun showSettingsDialog() {
        val bottomSheetDialog = BottomSheetDialog(this, R.style.Theme_SmartFridge_NoActionBar) // Use current theme
        val view = layoutInflater.inflate(R.layout.dialog_settings, null)
        bottomSheetDialog.setContentView(view)
        
        // Ensure transparent background for rounded corners to show
        (view.parent as View).setBackgroundColor(Color.TRANSPARENT)

        val btnLogout = view.findViewById<Button>(R.id.btnLogout)
        val etUserName = view.findViewById<android.widget.EditText>(R.id.etUserName)
        val btnSaveName = view.findViewById<Button>(R.id.btnSaveName)
        
        val prefs = getSharedPreferences("smart_fridge_prefs", Context.MODE_PRIVATE)
        etUserName.setText(prefs.getString("user_name", ""))

        // Handle Save Name
        btnSaveName.setOnClickListener {
            val name = etUserName.text.toString().trim()
            if (name.isNotEmpty()) {
                prefs.edit().putString("user_name", name).apply()
                updateHelloUserText(name)
                showCustomToast("NAME SAVED SUCCESSFULLY!")
                bottomSheetDialog.dismiss()
            } else {
                showCustomToast("PLEASE ENTER A VALID NAME")
            }
        }
        
        // Handle Logout
        btnLogout.setOnClickListener {
            bottomSheetDialog.dismiss()
            val prefs = getSharedPreferences("smart_fridge_prefs", Context.MODE_PRIVATE)
            prefs.edit().clear().apply()
            
            mqttClient?.disconnect()

            startActivity(Intent(this, ConfigActivity::class.java))
            @Suppress("DEPRECATION")
            overridePendingTransition(R.anim.slide_in_right, R.anim.slide_out_left)
            finish()
        }
        
        bottomSheetDialog.show()
    }
    
    private fun showCustomToast(message: String) {
        val snackbar = com.google.android.material.snackbar.Snackbar.make(
            findViewById(android.R.id.content),
            message.uppercase(),
            com.google.android.material.snackbar.Snackbar.LENGTH_SHORT
        )
        // Cyberpunk background and premium text color
        snackbar.view.setBackgroundResource(R.drawable.bg_cyberpunk_glass)
        snackbar.setTextColor(Color.parseColor("#40C4FF")) // Accent cyan
        snackbar.show()
    }
}
