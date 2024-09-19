import { viteBundler } from '@vuepress/bundler-vite'
import { defaultTheme } from '@vuepress/theme-default'
import { defineUserConfig } from 'vuepress'
import { gungnirTheme } from 'vuepress-theme-gungnir'
// import '@vuepress/theme-blog'
// export default defineUserConfig({
//   bundler: viteBundler()
// })
// const { gungnirTheme } = require("vuepress-theme-gungnir");
module.exports = {
    bundler:viteBundler(),
    title: 'My blog ',
    theme: gungnirTheme({
        // you theme configs
      }),
    themeConfig:{

    }
}