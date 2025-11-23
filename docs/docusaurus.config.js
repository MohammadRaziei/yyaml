import {themes as prismThemes} from 'prism-react-renderer';

// This runs in Node.js - Don't use client-side code here (browser APIs, JSX...)

/** @type {import('@docusaurus/types').Config} */

// Source of truth for the yyaml docs site; copied into build/docusaurus during make.

const config = {
  title: 'yyaml',
  tagline: 'High-performance YAML parsing in C with C++ bindings',
  url: 'https://mohammadraziei.github.io',
  baseUrl: '/yyaml',
  trailingSlash: false,
  onBrokenLinks: 'warn',
  onBrokenMarkdownLinks: 'warn',
  organizationName: 'yyaml',
  projectName: 'yyaml-docs',
  staticDirectories: [],
  markdown: {
    format: 'mdx',
  },
  presets: [
    [
      'classic',
      /** @type {import('@docusaurus/preset-classic').Options} */
      ({
        docs: false,
        blog: false,
        theme: {
          customCss: './src/css/custom.css',
        },
      }),
    ],
  ],

  themeConfig:
    /** @type {import('@docusaurus/preset-classic').ThemeConfig} */
    ({
      // Replace with your project's social card
      image: 'img/docusaurus-social-card.jpg',
      colorMode: {
        defaultMode: 'light',
        respectPrefersColorScheme: false,
      },
      navbar: {
        title: 'yyaml',
        logo: {
          alt: 'yyaml Logo',
          src: 'img/yyaml.svg',
          width: 32,
          height: 32,
        },
        items: [
        { to: '/', label: 'Overview', position: 'left' },
        { to: '/languages', label: 'Languages', position: 'left' },
        { to: '/benchmarks', label: 'Benchmarks', position: 'left' },
        { href: 'https://github.com/mohammadraziei/yyaml', label: 'GitHub', position: 'right' },
        ],
      },
      footer: {
        style: 'dark',
        links: [
          {
            title: 'Docs',
            items: [
            { label: 'Overview', to: '/' },
            { label: 'C API', to: '/languages/c' },
            { label: 'C++ API', to: '/languages/cpp' },
            ],
          },
          {
            title: 'Community',
            items: [
            { label: 'GitHub Issues', href: 'https://github.com/mohammadraziei/yyaml/issues' },
            ],
          },
        ],
      copyright: `Copyright © ${new Date().getFullYear()} yyaml.`,
      },
      prism: {
        theme: prismThemes.github,
        darkTheme: prismThemes.dracula,
      },
    }),
};

export default config;
