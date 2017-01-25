




var queProyectorNecEs;
var queProyectorVSEs;
var controlNEC;
var urlProyector;
var result;
var result1;
var result2;
var result3;

var isProjectNec1_On=false;  //Nec 1
var isProjectNec2_On=false;  //Nec 2
var isProjectVS1_On =false;  //VS  1
var isProjectVS2_On =false;  //VS  2


$(document).ready(function(){

//  StatusProjectNEC();


//***** Proyector NEC 1  ON/OFF
  $("#btn1").click(function(){
    if(!isProjectNec1_On){
      $("#btn1").attr("src","NEC_MID.png")
      console.log("Prender NEC 1");
      // window.alert ("On");
      ON_NEC1();
    }
    else{
      $("#btn1").attr("src","NEC_MID.png")
      console.log("Apagar NEC 1")
      OFF_NEC1();
    }
  });

//***** Proyector NEC 2  ON/OFF
  $("#btn2").click(function(){
    if(!isProjectNec2_On){
      $("#btn2").attr("src","NEC_MID.png")
      console.log("Prender NEC 2");
      ON_NEC2();
    }
    else{
      $("#btn2").attr("src","NEC_MID.png")
      console.log("Apagar NEC 2")
      OFF_NEC2();
    }
  })

//***** Proyector View Sonic 1  ON/OFF
  $("#btn3").click(function(){
    queProyectorVSEs = 1;
    if(!isProjectVS1_On){
      $("#btn3").attr("src","Vsonic_ON.png")
      console.log("Prender ViewSonic 1");
      urlProyector = "/ON"+queProyectorEs;
      ON();
      isProjectVS1_On=!isProjectVS1_On;
    }
    else{
      $("#btn3").attr("src","Vsonic_OFF.png")
      console.log("Apagar ViewSonic 1")
      urlProyector = "/OFF"+queProyectorEs;
      OFF();
      isProject3_On=!isProject3_On;
    }
  })

//***** Proyector View Sonic 2  ON/OFF
  $("#btn4").click(function(){
    queProyectorEs = 2;
    if(!isProjectVS2_On){
      $("#btn4").attr("src","Vsonic_ON.png")
      console.log("Prender ViewSonic 2");
      urlProyector = "/ON"+queProyectorEs;
      ON();
      isProjectVS2_On=!isProjectVS2_On;
    }
    else{
      $("#btn4").attr("src","Vsonic_OFF.png")
      console.log("Apagar ViewSonic 2")
      urlProyector = "/OFF"+queProyectorEs;
      OFF();
      isProjectVS2_On=!isProjectVS2_On;
    }
  })  

});


//******* Envío de peticiones Http con Ajax

function StatusProjectNEC(){
   $.ajax({
    url: "StatusRunningRequest"
  }).done(function(ResponseType){

    if (ResponseType == '0'){
      console.log("No hay conexión Uc - Proyector")
    } else if (ResponseType == '1'){
      console.log("Missmatch-BaudRateFail")
    } else if (ResponseType == '2'){
      console.log("Projector ON/OFF")
      $("#btn1").attr("src","NEC_ON.png")
      isProjectNEC1_On = !isProjectNEC1_On;
    } else if (ResponseType == '3'){
      console.log("PowerSatatus ON")
    } else if (ResponseType == '4'){
      console.log("PowerSatatus StandBy")
    } else if (ResponseType == '5'){
      console.log("Error - Projector OFF")
    } else {
      console.log("El comando no puede ser reconocido")
    }

  });
};



function ON_NEC1(){
  $.ajax({
    url: "/ON1"
  }).done(function(result){
	console.log(result);
    if (result == '0'){
      console.log("No hay conexión Uc - Proyector")
    } else if (result == '1'){
      console.log("Missmatch-BaudRateFail")
    } else if (result == '2'){

      console.log("Projector NEC1 ON");
     	$("#btn1").attr("src","NEC_ON.png");
      isProjectNec1_On = !isProjectNec1_On;

    } else if (result == '3'){
      console.log("PowerSatatus ON")
    } else if (result == '4'){
      console.log("PowerSatatus StandBy")
    } else if (result == '5'){
      console.log("Error - Projector OFF")
    } else {
      console.log("El comando no puede ser reconocido")
    }

  });
};


function OFF_NEC1(){
  $.ajax({
    url: "/OFF1"
  }).done(function(result1){
	console.log(result1);
    if (result1 == '0'){
      console.log("No hay conexión Uc - Proyector")
    } else if (result1 == '1'){
      console.log("Missmatch-BaudRateFail")
    } else if (result1 == '2'){

      console.log("Projector NEC1 OFF");
     	$("#btn1").attr("src","NEC_OFF.png");
      isProjectNec1_On = !isProjectNec1_On;

    } else if (result1 == '3'){
      console.log("PowerSatatus ON")
    } else if (result1 == '4'){
      console.log("PowerSatatus StandBy")
    } else if (result1 == '5'){
      console.log("Error - Projector OFF")
    } else {
      console.log("El comando no puede ser reconocido")
    }

  });
};

function ON_NEC2(){
  $.ajax({
    url: "/ON2"
  }).done(function(result2){
  console.log(result2);
    if (result2 == '0'){
      console.log("No hay conexión Uc - Proyector")
    } else if (result2 == '1'){
      console.log("Missmatch-BaudRateFail")
    } else if (result2 == '2'){

      console.log("Projector NEC2 ON");
      $("#btn2").attr("src","NEC_ON.png");
      isProjectNec2_On = !isProjectNec2_On;

    } else if (result2 == '3'){
      console.log("PowerSatatus ON")
    } else if (result2 == '4'){
      console.log("PowerSatatus StandBy")
    } else if (result2 == '5'){
      console.log("Error - Projector OFF")
    } else {
      console.log("El comando no puede ser reconocido")
    }

  });
};


function OFF_NEC2(){
  $.ajax({
    url: "/OFF2"
  }).done(function(result3){
  console.log(result3);
    if (result3 == '0'){
      console.log("No hay conexión Uc - Proyector")
    } else if (result3 == '1'){
      console.log("Missmatch-BaudRateFail")
    } else if (result3 == '2'){

      console.log("Projector NEC2 OFF");
      $("#btn2").attr("src","NEC_OFF.png");
      isProjectNec2_On = !isProjectNec2_On;

    } else if (result3 == '3'){
      console.log("PowerSatatus ON")
    } else if (result3 == '4'){
      console.log("PowerSatatus StandBy")
    } else if (result3 == '5'){
      console.log("Error - Projector OFF")
    } else {
      console.log("El comando no puede ser reconocido")
    }

  });
};





