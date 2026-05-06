<script setup>
// Імпортуємо твій розроблюваний компонент
import Anki from './components/Anki.vue';
import Abobus from './components/Greetings.vue'
import Protocols from './components/Protocols.vue';
</script>

<template>
  <div class="v-application v-theme--nrdb nrdb-app" id="app">
    <!-- Прибрали старий інлайн-фон, тепер працює картинка з body::before -->
    <main class="v-main" style="min-height: 100vh;">
      
      <!-- Сторінка -->
      <div class="nrdb-layout--grid nrdb-ui-page">
        
        <!-- Група -->
        <div class="nrdb-ui-group" style="grid-column-end: span min(16, var(--layout-columns));">
          
          <!-- СКЛЯНА ОБГОРТКА ГРУПИ 
               (Отримує стилі скла автоматично, бо це .v-card без .nrdb-ui-template) -->
          <div class="v-card v-theme--nrdb v-card--density-comfortable bg-group-background">
            <div class="v-card-text">
              
              <!-- Сітка групи -->
              <div class="nrdb-layout-group--grid" style="grid-template-columns: repeat(min(16, var(--layout-columns)), 1fr);">
                
                <!-- ПРОЗОРИЙ ВІДЖЕТ 
                     (Залишається прозорим завдяки класу .nrdb-ui-template, який виключається в CSS) -->
                <div 
                  class="nrdb-ui-widget nrdb-ui-template" 
                  style="
                    grid-row-end: span 8; 
                    grid-column-end: span min(5, var(--layout-columns));
                  "
                >
                  <!-- ТВІЙ КОМПОНЕНТ -->
                  <Anki />
                  <Abobus />
                  <Protocols />
                </div>

              </div>
            </div>
          </div>
        </div>
        
      </div>
    </main>
  </div>
</template>

<style>
  @import url('https://fonts.googleapis.com/css2?family=Source+Code+Pro:ital,wght@0,200..900;1,200..900&display=swap');

  /* =========================================
  0. ЗМІННІ КОЛЬОРІВ (ТУТ ВСЯ МАГІЯ)
  ========================================= */
  
  :root {
      /* Лейаут */
      --layout-columns: 12;
      --widget-row-height: 48px;
      --widget-gap: 12px;
      --group-gap: 12px;
      --page-padding: 30px 40px 0 40px;

      /* Текст */
      --color-text-main: #f0f0f0;
      --color-text-strong: #ffffff;
      
      /* Скляні панелі */
      --glass-bg: rgba(20, 20, 25, 0.45);
      --glass-border: rgba(255, 255, 255, 0.08);
      --glass-border-top: rgba(255, 255, 255, 0.15);
      --glass-shadow: rgba(0, 0, 0, 0.3);
      --glass-shadow-hover: rgba(0, 0, 0, 0.5);
      
      /* Інпути */
      --input-bg: rgba(0, 0, 0, 0.25);
      --input-border: rgba(255, 255, 255, 0.05);
      --input-shadow: rgba(0, 0, 0, 0.2);
      
      /* Кнопка */
      --btn-gradient-start: rgb(240, 150, 20);
      --btn-gradient-end: rgb(220, 100, 10);
      --btn-shadow: rgba(240, 133, 10, 0.4);
      --btn-border: rgba(255, 255, 255, 0.2);
      
      /* Ховер */
      --hover-border: rgba(255, 255, 255, 0.25);
      
      /* Тіні тексту */
      --text-shadow: rgba(0, 0, 0, 0.6);
  }

  /* =========================================
  1. ФОН
  ========================================= */
  body::before {
      content: "";
      position: fixed;
      top: 0;
      left: 0;
      width: 100vw;
      height: 100vh;
      
      background-image: url('https://i.pinimg.com/1200x/fb/d9/0d/fbd90da92ebafa680dd8729e3aea2ab7.jpg');
      background-size: cover;
      background-position: center;
      background-attachment: fixed;
      
      filter: blur(7px) brightness(0.5);
      z-index: -1;
      transform: scale(1.1);
  }

  /* =========================================
     2. БАЗА
     ========================================= */

  /* Забороняємо горизонтальний скрол на рівні всієї сторінки */
  html, body {
    overflow-x: hidden !important;
    max-width: 100% !important;
  }
  
  /* Фіксимо можливе розширення головного контейнера */
  #app, .v-application, .v-application__wrap, .v-main {
    max-width: 100% !important;
    overflow-x: hidden !important;
  }
  
  /* Змушуємо ВСІ елементи враховувати padding у свою ширину */
  *, *::before, *::after {
    box-sizing: border-box !important;
  }

  body,
  .v-application,
  .v-application__wrap,
  .v-main,
  .nrdb-app,
  .bg-group-background,
  .v-navigation-drawer {
      background: transparent !important;
      background-color: transparent !important;
      color: var(--color-text-main) !important;
      font-family: "Source Code Pro", monospace !important;
      font-weight: 400 !important;
  }

  html, body {
      overflow: auto;
      scrollbar-width: none;
  }
  
  body::-webkit-scrollbar {
      display: none;
  }

  .nrdb-ui-page {
      padding: var(--page-padding) !important;
  }

  /* Імітація Гріда Сторінки */
  .nrdb-layout--grid {
    display: grid;
    grid-template-columns: repeat(var(--layout-columns), 1fr);
    gap: var(--group-gap);
  }

  /* Імітація Гріда Групи */
  .nrdb-layout-group--grid {
    display: grid;
    gap: var(--widget-gap);
  }

  .v-card-text {
    padding: 0; 
  }

  /* =========================================
     3. GLASS UI (ГРУПИ)
     ========================================= */
  .v-card:not(.nrdb-ui-template),
  .nrdb-ui-widget:not(.nrdb-ui-template),
  .v-toolbar:not(.nrdb-ui-template) {
      background: var(--glass-bg) !important;
      backdrop-filter: blur(8px) saturate(180%) !important;
      -webkit-backdrop-filter: blur(8px) saturate(180%) !important;

      border: 1px solid var(--glass-border) !important;
      border-top: 1px solid var(--glass-border-top) !important;

      border-radius: 16px !important;
      box-shadow: 0 8px 32px 0 var(--glass-shadow) !important;
      color: var(--color-text-strong) !important;

      padding: 16px;
      margin: 10px;

      transition: transform 0.3s cubic-bezier(0.25, 0.8, 0.25, 1),
          box-shadow 0.3s cubic-bezier(0.25, 0.8, 0.25, 1),
          border-color 0.3s ease !important;
  }

  /* =========================================
     4. ХОВЕР
     ========================================= */
  .nrdb-ui-widget:hover:not(.nrdb-ui-template) {
      box-shadow: 0 12px 40px 0 var(--glass-shadow-hover) !important;
      border-color: var(--hover-border) !important;
  }

  /* =========================================
     5. ТЕКСТ ТА ЕЛЕМЕНТИ
     ========================================= */
  .v-card-title,
  .v-toolbar-title {
      font-weight: 700 !important;
      letter-spacing: 1px !important;
      text-shadow: 0 2px 4px var(--text-shadow);
  }

  .v-field__outline {
      display: none !important;
  }

  .v-field {
      background: var(--input-bg) !important;
      border-radius: 10px !important;
      border: 1px solid var(--input-border) !important;
      box-shadow: inset 0 2px 4px var(--input-shadow) !important;
      color: var(--color-text-strong) !important;
  }

  .v-color-picker {
      background: var(--input-bg) !important;
  }

  .v-color-picker * {
      color: var(--color-text-strong) !important;
  }

  .v-btn--variant-flat {
      background: linear-gradient(135deg, var(--btn-gradient-start) 0%, var(--btn-gradient-end) 100%) !important;
      color: var(--color-text-strong) !important;
      font-weight: 800 !important;
      letter-spacing: 1px !important;
      border-radius: 10px !important;
      border: 1px solid var(--btn-border) !important;
      box-shadow: 0 4px 15px var(--btn-shadow) !important;
      text-transform: uppercase !important;
  }
</style>