import QtQuick 2.0

Canvas {
	id: canvas
	antialiasing: true
	renderStrategy: Canvas.Cooperative /* keep rendering in the main thread ! */
	renderTarget: Canvas.FramebufferObject
	anchors.fill: parent

	property int clientID: -1

	property int power_range_high: 40
	property int power_range_low: -10
	property real freq_low:  0
	property real freq_high: 32000
	property real time_scale: 1000000000 /* 1000 ms */
	property real time: 0

	property int margin_top: 10
	property int fft_top: canvas.height*8.5/10
	property int margin_bottom: 10
	property int margin_left: 10
	property int margin_right: 10
	property int i:0

	Component.onCompleted: {

	}

	MouseArea {
		id: mouseArea
		anchors.fill: parent
		hoverEnabled: true

		/*onPositionChanged: {
			console.log("mouse moved, x = " + mouse.x + ", y = " + mouse.y)
		}

		onClicked: {
			console.log("clicked at x = " + mouse.x + ", y = " + mouse.y)
		}*/
	}

	Button {
		property bool paused: false

		anchors { right: parent.right; top: parent.top}
		text: { return paused?"Unpause":"Pause"; }
		onClicked: {
			paused = !paused;

			var SensingNode = SensingServer.sensingNode(clientID)
			SensingNode.pauseUpdates(paused);
		}
	}

	function value_normalize(value, norm_value, toHigher)
	{
		if (value % norm_value != 0)
		{
			value = Math.floor(value / norm_value) * norm_value
			if (toHigher === 1)
				value += norm_value
		}

		return value
	}

	function biggestSize(ctx, str1, str2, str3)
	{
		var str1_size = ctx.measureText(str1)
		var str2_size = ctx.measureText(str2)
		var str3_size = ctx.measureText(str3)

		if (str1_size.width > str2_size.width)
			if (str1_size.width > str3_size.width)
				return str1_size
			else
				return str3_size
		else
			if (str2_size.width > str3_size.width)
				return str2_size
			else
				return str3_size
	}

	function freqToText(freq)
	{
		var units = ["Hz", "kHz", "MHz", "GHz", "THz"]
		var unit = 0

		while (freq > 1000 && unit < 5)
		{
			freq /= 1000;
			unit++;
		}

		return freq.toFixed(2) + " " + units[unit]
	}

	function timeToText(time)
	{
		var units = ["ns", "µs", "ms", "s"]
		var unit = 0

		while (Math.abs(time) >= 1000 && unit < 4)
		{
			time /= 1000;
			unit++;
		}

		return time.toFixed(2) + " " + units[unit]
	}

	function getCoordinatesPreCache(ctx)
	{
		var font_height = 8
		var y_space_coms_graph = 10;
		var dB_bar_text_size = biggestSize(ctx, canvas.power_range_low, canvas.power_range_high, timeToText(time_scale))
		var dB_bar_x_offset = canvas.margin_left + font_height + dB_bar_text_size.width
		var freq_bar_top_y_offset_fft = font_height
		var freq_bar_bottom_y_offset_fft = 2 * (font_height + 10)	// 5 for the mark and 3 for the spacing
		var graph_width = canvas.width - dB_bar_x_offset - canvas.margin_right
		var coms_height = canvas.fft_top - y_space_coms_graph - canvas.margin_top - 5
		var graph_height = canvas.height - canvas.fft_top - freq_bar_top_y_offset_fft - freq_bar_bottom_y_offset_fft - canvas.margin_bottom
		var freq_range = canvas.freq_high - canvas.freq_low
		var power_range = canvas.power_range_low - canvas.power_range_high
		var time_start = canvas.time - canvas.time_scale
		var time_end = canvas.time
		var time_range = canvas.time_scale

		var cache = {
			font_height: font_height,
			dB_bar_x_offset: dB_bar_x_offset,
			coms_width: graph_width,
			coms_height: coms_height,
			graph_width: graph_width,
			graph_height: graph_height,
			freq_range: freq_range,
			power_range: power_range,
			time_start: time_start,
			time_end: time_end,
			time_range: time_range,
			x_offset: dB_bar_x_offset,
			y_offset_coms: canvas.margin_top,
			y_offset_coms_bottom: canvas.margin_top + coms_height,
			y_offset_fft: canvas.fft_top + font_height,
			x_factor: graph_width / freq_range,
			fft_y_factor: graph_height / power_range,
			coms_y_factor: coms_height / time_range
		}

		return cache;
	}

	function getComsCoordinates(ctx, cache, freq, time)
	{
		if (time < cache.time_start)
			time = cache.time_start
		else if (time > cache.time_end)
			time = cache.time_end

		var timeOffset = time - cache.time_end
		var pos = {
			x: cache.x_offset + cache.x_factor * (freq - canvas.freq_low),
			y: cache.y_offset_coms_bottom + cache.coms_y_factor * timeOffset,
			toString: function () {
				return "[" + this.x + "," + this.y + "]"
			}
		}

		return pos
	}

	function getFftCoordinates(ctx, cache, freq, power)
	{
		var pos = {
			x: cache.x_offset + cache.x_factor * (freq - canvas.freq_low),
			y: cache.y_offset_fft + cache.fft_y_factor * (power - canvas.power_range_high),
			toString: function () {
				return "[" + this.x + "," + this.y + "]"
			}
		}

		return pos
	}

	function frequencyResolution(frequencyStart, frequencyEnd)
	{
		var minPixelsPerFrequency = 100
		var units = [1, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000]
		var id = 0

		while (id < units.length)
		{
			var unit = units[id] * 1000
			var freqStartRound = Math.ceil(frequencyStart / unit) * unit
			var freqEndRound = Math.ceil(frequencyEnd / unit) * unit

			var freqRoundRange = freqEndRound - freqStartRound
			var freqCount = freqRoundRange / unit
			if (canvas.width / freqCount > minPixelsPerFrequency)
			{
				var i = 0

				var frequencies = new Array();
				while (i < freqCount)
				{
					frequencies[i] = freqStartRound + i * unit
					i++
				}

				return frequencies;
			}

			id++
		}
	}

	function isPointInRectangle(x, y, rectTopLeft, rectBottomRight)
	{
		return x >= rectTopLeft.x && x <= rectBottomRight.x && y >= rectTopLeft.y && y <= rectBottomRight.y

	}

	function showTooltip(ctx, tooltipText, x, y)
	{
		if (tooltipText.length > 0) {
			var posOffset = 4
			var tooltipMargin = 4;
			var lineHeight = 15
			var tooltipHeight = tooltipText.length * lineHeight + 2 * tooltipMargin
			var tooltipX = x + posOffset
			var tooltipY = (y - posOffset) - tooltipHeight

			var tooltipWidth = 0
			for (var i = 0; i < tooltipText.length; i++) {
				var lineWidth = ctx.measureText(tooltipText[i]).width + 2 * tooltipMargin
				if (lineWidth > tooltipWidth)
					tooltipWidth = lineWidth;
			}

			/* make sure we don't go out of the component */
			if (tooltipX + tooltipWidth > canvas.width)
				tooltipX = canvas.width - tooltipWidth
			if (tooltipY < 0)
				tooltipY = 0

			ctx.fillStyle = Qt.rgba(0.37, 0.37, 0.82, 0.6)
			ctx.beginPath()
			ctx.rect(tooltipX, tooltipY, tooltipWidth, tooltipHeight)
			ctx.fill()
			ctx.closePath()

			ctx.fillStyle = "black"
			ctx.textAlign = "left"
			ctx.textBaseline = "bottom"
			ctx.beginPath()
			for (var i = 0; i < tooltipText.length; i++) {
				tooltipY += lineHeight
				ctx.fillText(tooltipText[i], tooltipX + tooltipMargin, tooltipY + tooltipMargin)
			}
			ctx.fill()
			ctx.closePath()
		}
	}

	function draw(ctx)
	{
		ctx.save();

		var SensingNode = SensingServer.sensingNode(clientID)
		SensingNode.fetchFftEntries(-1, 0);
		var pos = SensingNode.getFftEntriesRange()["start"];
		var spectrum = SensingNode.selectFftEntry(pos);

		time = spectrum.timeNs();

		var gc_cache = getCoordinatesPreCache(ctx)

		var g_top_left = getFftCoordinates(ctx, gc_cache, freq_low, canvas.power_range_high)
		var g_bottom_right = getFftCoordinates(ctx, gc_cache, freq_high, canvas.power_range_low)

		var c_top_left = getComsCoordinates(ctx, gc_cache, freq_low, time - canvas.time_scale)
		var c_bottom_right = getComsCoordinates(ctx, gc_cache, freq_high, time)

		ctx.font = "14px sans-serif"

	// draw the axis
		ctx.strokeStyle = "black"
		ctx.lineWidth = 2
		ctx.beginPath();

		ctx.moveTo(g_top_left.x, gc_cache.y_offset_coms);
		ctx.lineTo(g_top_left.x, c_bottom_right.y)
		ctx.lineTo(c_bottom_right.x, c_bottom_right.y)
		ctx.stroke()

		ctx.moveTo(g_top_left.x, g_top_left.y);
		ctx.lineTo(g_top_left.x, g_bottom_right.y)
		ctx.lineTo(g_bottom_right.x, g_bottom_right.y)
		ctx.stroke()
		ctx.closePath()

	// draw the inverted labels for the axis
		ctx.save();
		ctx.fillStyle = "black"
		ctx.rotate(-Math.PI/4);
		ctx.textAlign = "center";

		var timeText = timeToText(time_scale);
		var timeLabel_y = gc_cache.y_offset_coms + gc_cache.coms_height / 2;
		ctx.fillText("Time (range = " + timeText + ")", -timeLabel_y, 20);
		ctx.fillText("t0 = " + time + "ns", -timeLabel_y, 20 + 2 * gc_cache.font_height)

		var powerLabel_y = gc_cache.y_offset_fft + gc_cache.graph_height / 2;
		ctx.fillText("Power (dBm)", -powerLabel_y, 20);

		ctx.restore();

	// draw the coms time labels
		ctx.beginPath()
		ctx.fillStyle = "black"
		ctx.textAlign = "right"
		ctx.lineWidth = 1.5

		var x = g_top_left.x
		var y = gc_cache.y_offset_coms
		ctx.moveTo(x - 5, y); ctx.lineTo(x, y); ctx.stroke();
		ctx.fillText(timeToText(-time_scale), x - 5, y + gc_cache.font_height / 2);

		y = gc_cache.y_offset_coms + gc_cache.coms_height
		ctx.moveTo(x - 5, y); ctx.lineTo(x, y); ctx.stroke();
		ctx.fillText("t0", g_top_left.x - 8, y + gc_cache.font_height / 2);
		ctx.closePath()

	// draw the dB lines
		ctx.beginPath()
		ctx.strokeStyle = "grey"
		ctx.lineWidth = 0.25
		ctx.fillStyle = "black"
		ctx.textAlign = "right"
		var lastDrawnTextY = 0
		for (var power = canvas.power_range_high; power >= canvas.power_range_low; power-=10)
		{
			var pos_start = getFftCoordinates(ctx, gc_cache, freq_low, power)
			var pos_end = getFftCoordinates(ctx, gc_cache, freq_high, power)

			if (pos_start.y - lastDrawnTextY > gc_cache.font_height) {
				lastDrawnTextY = pos_start.y + gc_cache.font_height / 2
				ctx.fillText(power, pos_start.x - 5, lastDrawnTextY)
				ctx.fill()
			}

			ctx.moveTo(pos_start.x, pos_start.y);
			ctx.lineTo(pos_end.x, pos_end.y)
			ctx.stroke()
		}
		ctx.closePath()

	// Coms
		var timeStart = canvas.time - canvas.time_scale
		var timeEnd = canvas.time
		var comCount = SensingNode.fetchCommunications(timeStart, timeEnd)
		//console.log("the are " + comCount + " communications from timeStart = " + gc_cache.time_start)
		ctx.lineWidth = 1
		for (var i = 0; i < comCount; i++)
		{
			var com = SensingNode.selectCommunication(i)
			var comTimeStart = com["timeStart"]
			var comTimeEnd = com["timeEnd"]
			var comFreqStart = com["frequencyStart"]
			var comFreqEnd = com["frequencyEnd"]
			var comPwr = com["pwr"]

			//console.log("	[" + comTimeStart + ", " + comTimeEnd + "]")

			var posTopLeft = getComsCoordinates(ctx, gc_cache, comFreqStart, comTimeEnd)
			var posBottomRight = getComsCoordinates(ctx, gc_cache, comFreqEnd, comTimeStart)
			var width = posBottomRight.x - posTopLeft.x
			var height = posBottomRight.y - posTopLeft.y

			ctx.beginPath()
			ctx.rect(posTopLeft.x, posTopLeft.y, width, height);
			ctx.fillStyle = 'yellow';
			ctx.fill();
			ctx.stroke()
			ctx.closePath();
		}
		SensingNode.freeCommunications()

	// FFT
		// draw the frequencies
		ctx.beginPath()
		ctx.textAlign = "center"
		ctx.textBaseline = "top"
		ctx.strokeStyle = "grey"
		ctx.fillStyle = "black"
		ctx.lineWidth = 0.25
		var frequencies = frequencyResolution(canvas.freq_low, canvas.freq_high)
		for (var i = 0; i < frequencies.length; i++)
		{
			var freq = frequencies[i]
			var pos = getFftCoordinates(ctx, gc_cache, freq, canvas.power_range_low)

			//console.log(freqToText(freq_low + i * (canvas.bandwidth / 10)))
			ctx.fillText(freqToText(freq), pos.x, pos.y + 8)
			ctx.fill()

			ctx.moveTo(pos.x, pos.y);
			ctx.lineTo(pos.x, canvas.margin_top)
			ctx.stroke()
		}
		ctx.fillText("Frequency of the signal", canvas.width / 2, pos.y + 27)
		ctx.fill()
		ctx.closePath()

		// draw the actual power state
		var gradientFill = ctx.createLinearGradient(g_top_left.x, g_top_left.y, g_top_left.x, g_bottom_right.y)
		gradientFill.addColorStop(0, Qt.rgba(1, 0, 0, 0.3));
		gradientFill.addColorStop(0.5, Qt.rgba(0, 0.8, 0, 0.3));
		gradientFill.addColorStop(1, Qt.rgba(0, 0, 0.8, 0.3));
		ctx.strokeStyle = "black"
		ctx.fillStyle = gradientFill
		ctx.lineWidth = 1.5
		ctx.beginPath()
		var size = spectrum.samplesCount();
		for (var i = 0; i < size; i++)
		{
			var freq = spectrum.sampleFrequency(i)
			var pwr = spectrum.sampleDbm(i)
			pos = getFftCoordinates(ctx, gc_cache, freq, pwr)

			if (i == 0)
				ctx.moveTo(pos.x, g_bottom_right.y)
			ctx.lineTo(pos.x, pos.y)
		}
		SensingNode.freeFftEntries()
		ctx.lineTo(pos.x, g_bottom_right.y)
		ctx.closePath()
		ctx.stroke()
		ctx.fill()

		/* display the tooltip */
		var mouseX = mouseArea.mouseX
		var mouseY = mouseArea.mouseY

		/* is mouse in the time area */
		if (isPointInRectangle(mouseX, mouseY, c_top_left, c_bottom_right))
		{
			var tooltipText = new Array()

			var offsetY = mouseY - c_top_left.y
			var timeOffset = offsetY * canvas.time_scale / (c_bottom_right.y - c_top_left.y)
			var mouseTime = time + timeOffset
			tooltipText[0] = "Time: " + mouseTime + " ns"

			var offsetX = mouseX - c_top_left.x
			var freqOffset = offsetX * (freq_high - freq_low) / (c_bottom_right.x - c_top_left.x)
			var mouseFreq = (freq_low + freqOffset) / 1000
			mouseFreq = mouseFreq.toFixed(0)
			tooltipText[1] = "Frequency: " + mouseFreq + " kHz"

			showTooltip(ctx, tooltipText, mouseX, mouseY)
		}

		/* is mouse in the fft area */
		else if (isPointInRectangle(mouseX, mouseY, g_top_left, g_bottom_right))
		{
			var tooltipText = new Array()

			var offsetY = mouseY - g_top_left.y
			var powerOffset = offsetY * (power_range_high - power_range_low) / (g_bottom_right.y - g_top_left.y)
			var mousePower = power_range_high - powerOffset
			tooltipText[0] = "Power: " + mousePower + " dBm"

			var offsetX = mouseX - g_top_left.x
			var freqOffset = offsetX * (freq_high - freq_low) / (g_bottom_right.x - g_top_left.x)
			var mouseFreq = (freq_low + freqOffset) / 1000
			mouseFreq = mouseFreq.toFixed(0)
			tooltipText[1] = "Frequency: " + mouseFreq + " kHz"

			showTooltip(ctx, tooltipText, mouseX, mouseY)
		}

		ctx.restore()
	}

	Connections {
		target: SensingServer.sensingNode(clientID)
		onDataChanged: {
			requestPaint()
		}
		onPowerRangeChanged: {
			power_range_high = high
			power_range_low = low
		}
		onFrequencyRangeChanged: {
			freq_high = high
			freq_low = low
		}
		onTimeChanged: {
			time = timeNs
		}
		onRequestDestroy: {
			canvas.destroy()
		}
	}

	/* try to refresh at around 60 fps */
	Timer {
			interval: 60; running: true; repeat: true;
			onTriggered: requestPaint();
		}

	onCanvasSizeChanged: requestPaint()
	onCanvasWindowChanged: requestPaint()

	onPaint: {
		var ctx = canvas.getContext('2d');
		ctx.save();

		// erase everything
		ctx.fillStyle = "white";
		ctx.fillRect(0, 0, canvas.width, canvas.height);

		draw(ctx);
		ctx.restore();

		/*if (i < 1024) {
			save("/tmp/heya_" + i + ".png");
			i++;
		}*/
	}
}
