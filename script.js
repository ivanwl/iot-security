var timeDelay = 0;

Highcharts.setOptions({
  global: {
    useUTC: false
  }
});

var chartT = new Highcharts.Chart({
  chart: { renderTo: "chart" },
  title: { text: "Activity" },
  series: [
    {
      showInLegend: false,
      data: []
    }
  ],
  plotOptions: {
    line: { animation: false, dataLabels: { enabled: true } },
    series: { color: "#059e8a" }
  },
  xAxis: { type: "datetime", dateTimeLabelFormats: { second: "%H:%M:%S" } },
  yAxis: {
    title: { text: "Activity" }
  },
  credits: { enabled: false }
});

setInterval(function() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var x = new Date().getTime();
      y = this.responseText.startsWith("No") ? 0 : 1;
      if (y === 1 && timeDelay < x) timeDelay = x + 10000;
      else y = 0;
      console.log(this.responseText);
      if (chartT.series[0].data.length > 40) {
        chartT.series[0].addPoint([x, y], true, true, true);
      } else {
        chartT.series[0].addPoint([x, y], true, false, true);
      }
    }
  };
  xhttp.open("GET", "/data", true);
  xhttp.send();
}, 500);
