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

		while (time >= 1000 && unit < 4)
		{
			time /= 1000;
			unit++;
		}

		return time.toFixed(2) + " " + units[unit]
	}

	function getCoordinatesPreCache(ctx)
	{
		var font_height = 10
		var y_space_coms_graph = 10;
		var dB_bar_text_size = biggestSize(ctx, canvas.power_range_low, canvas.power_range_high, timeToText(time_scale))
		var dB_bar_x_offset = canvas.margin_left + dB_bar_text_size.width + 5
		var freq_bar_top_y_offset_fft = font_height
		var freq_bar_bottom_y_offset_fft = font_height + 10	// 5 for the mark and 3 for the spacing
		var graph_width = canvas.width - dB_bar_x_offset - canvas.margin_right
		var coms_height = canvas.fft_top - y_space_coms_graph - canvas.margin_top
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
		var g_bottom_left = getFftCoordinates(ctx, gc_cache, freq_low, canvas.power_range_low)
		var g_bottom_right = getFftCoordinates(ctx, gc_cache, freq_high, canvas.power_range_low)

		var c_bottom_right = getComsCoordinates(ctx, gc_cache, freq_high, time)

		// draw the dB lines
		ctx.beginPath()
		ctx.strokeStyle = "grey"
		ctx.lineWidth = 0.25
		ctx.fillStyle = "black"
		ctx.textAlign = "right"
		ctx.textBaseline = "middle"
		for (var power = canvas.power_range_high; power >= canvas.power_range_low; power-=10)
		{
			var pos_start = getFftCoordinates(ctx, gc_cache, freq_low, power)
			var pos_end = getFftCoordinates(ctx, gc_cache, freq_high, power)

			ctx.fillText(power, pos_start.x - 5, pos_start.y)
			ctx.fill()

			ctx.moveTo(pos_start.x, pos_start.y);
			ctx.lineTo(pos_end.x, pos_end.y)
			ctx.stroke()
		}
		ctx.closePath()

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
			/*ctx.fillStyle = 'yellow';
			ctx.fill();*/
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
		var freq_range = canvas.freq_high - canvas.freq_low
		for (var i = 0; i < 10; i++)
		{
			var freq = freq_low + i * Math.floor(freq_range / 9)
			var pos = getFftCoordinates(ctx, gc_cache, freq, canvas.power_range_low)

			//console.log(freqToText(freq_low + i * (canvas.bandwidth / 10)))
			ctx.fillText(freqToText(freq), pos.x, pos.y + 8)
			ctx.fill()

			ctx.moveTo(pos.x, pos.y);
			ctx.lineTo(pos.x, canvas.margin_top)
			ctx.stroke()

		}
		ctx.closePath()

		// draw the actual power state
		var gradientFill = ctx.createLinearGradient(g_top_left.x, g_top_left.y, g_bottom_left.x, g_bottom_left.y)
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
				ctx.moveTo(pos.x, g_bottom_left.y)
			ctx.lineTo(pos.x, pos.y)
		}
		SensingNode.freeFftEntries()
		ctx.lineTo(pos.x, g_bottom_right.y)
		ctx.closePath()
		ctx.stroke()
		ctx.fill()

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

		// set the current time
		ctx.fillStyle = "black"
		ctx.fillText("t0 = " + time + "ns", canvas.width - 230, margin_top + 3)

		draw(ctx);
		ctx.restore();

		/*if (i < 1024) {
			save("/tmp/heya_" + i + ".png");
			i++;
		}*/
	}
}
