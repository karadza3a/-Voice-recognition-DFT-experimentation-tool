var schart;
var dchart;
var specchart;

function generateSongChart (chartData) {
  schart = AmCharts.makeChart("songchartdiv", {
    type: "serial",
    dataProvider: chartData,
    categoryField: "time",
    categoryAxis: {
      parseDates: false,
      gridAlpha: 0.15,
      minorGridEnabled: true,
      axisColor: "#DADADA",
    },
    valueAxes: [{
      axisAlpha: 0.2,
      id: "v1"
    }],
    graphs: [{
      title: "red line",
      id: "g1",
      valueAxis: "v1",
      valueField: "value",
      bullet: "none",
      bulletBorderColor: "#FFFFFF",
      bulletBorderAlpha: 1,
      lineThickness: 2,
      lineColor: "#b5030d",
      negativeLineColor: "#0352b5",
      balloonText: "[[category]]<br><b><span style='font-size:14px;'>value: [[value]]</span></b>"
    }],
    chartCursor: {
      fullWidth:true,
      cursorAlpha:0.1
    },
    chartScrollbar: {
      scrollbarHeight: 40,
      color: "#FFFFFF",
      autoGridCount: true,
      graph: "g1"
    },

    mouseWheelZoomEnabled:true
  });
  //schart.zoomToIndexes(0, chartData.length - 1);
}
function generateDftChart (chartData) {
  dchart = AmCharts.makeChart("dftchartdiv", {
    type: "serial",
    dataProvider: chartData,
    categoryField: "time",
    categoryAxis: {
      parseDates: false,
      gridAlpha: 0.15,
      minorGridEnabled: true,
      axisColor: "#DADADA",
    },
    valueAxes: [{
      axisAlpha: 0.2,
      id: "v1"
    }],
    graphs: [{
      title: "red line",
      id: "g1",
      valueAxis: "v1",
      valueField: "value",
      bullet: "none",
      bulletBorderColor: "#FFFFFF",
      bulletBorderAlpha: 1,
      lineThickness: 2,
      lineColor: "#b5030d",
      negativeLineColor: "#0352b5",
      balloonText: "[[category]]<br><b><span style='font-size:14px;'>value: [[value]]</span></b>"
    }],
    chartCursor: {
      fullWidth:true,
      cursorAlpha:0.1
    },
    chartScrollbar: {
      scrollbarHeight: 40,
      color: "#FFFFFF",
      autoGridCount: true,
      graph: "g1"
    },

    mouseWheelZoomEnabled:true
  });
  dchart.zoomToIndexes(0, chartData.length - 1);
}

function generateSpectre (chartData) {

  specchart = new AmCharts.AmSerialChart();
  specchart.dataProvider = chartData;
  specchart.categoryField = "frequency";

  var categoryAxis = specchart.categoryAxis;
  categoryAxis.labelRotation = 90;
  categoryAxis.gridPosition = "start";

  var graph = new AmCharts.AmGraph();
  graph.valueField = "magnitude";
  graph.balloonText = "[[frequency]]: <b>[[magnitude]]</b>";
  graph.type = "column";
  graph.lineAlpha = 0;
  graph.fillAlphas = 0.8;
  specchart.addGraph(graph);

  var chartCursor = new AmCharts.ChartCursor();
  chartCursor.cursorAlpha = 0.5;
  chartCursor.zoomable = true;
  chartCursor.categoryBalloonEnabled = false;
  specchart.addChartCursor(chartCursor);

  specchart.creditsPosition = "top-left";

  specchart.write("spectrediv");
}

function changeSource () {
  if ($("#inputType").val() == "File") {
    $("#sineSettings").hide();
    $("#fileSettings").show();
  }else{
    $("#sineSettings").show();
    $("#fileSettings").hide();
  }
}